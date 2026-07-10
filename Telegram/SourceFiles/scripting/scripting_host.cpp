/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "scripting/scripting_host.h"

#include "settings.h" // cWorkingDir
#include "main/main_session.h"
#include "apiwrap.h"
#include "api/api_common.h"
#include "data/data_session.h"
#include "data/data_histories.h"
#include "data/data_peer.h"
#include "data/data_media_types.h"
#include "data/data_document.h"
#include "data/data_photo.h"
#include "data/data_photo_media.h"
#include "data/data_file_origin.h"
#include "dialogs/dialogs_main_list.h"
#include "dialogs/dialogs_indexed_list.h"
#include "dialogs/dialogs_row.h"
#include "history/history.h"
#include "history/history_item.h"
#include "history/view/history_view_element.h"
#include "ui/toast/toast.h"
#include "base/debug_log.h"

#include <quickjs.h>

#include <QtCore/QDir>

namespace Scripting {
namespace {

[[nodiscard]] HostContext *GetHost(JSContext *ctx) {
	return static_cast<HostContext*>(JS_GetContextOpaque(ctx));
}

[[nodiscard]] PeerData *CurrentPeer(JSContext *ctx) {
	const auto host = GetHost(ctx);
	if (!host || !host->session || !host->peerId) {
		return nullptr;
	}
	return host->session->data().peerLoaded(PeerId(host->peerId));
}

JSValue TgLog(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv) {
	if (argc >= 1) {
		const auto raw = JS_ToCString(ctx, argv[0]);
		if (raw) {
			LOG(("Script: %1").arg(QString::fromUtf8(raw)));
			JS_FreeCString(ctx, raw);
		}
	}
	return JS_UNDEFINED;
}

JSValue TgCurrentChat(JSContext *ctx, JSValueConst, int, JSValueConst *) {
	const auto host = GetHost(ctx);
	const auto peer = CurrentPeer(ctx);
	if (!peer) {
		return JS_NULL;
	}
	auto object = JS_NewObject(ctx);
	// id as a string: PeerId packs a type tag in high bits, which would lose
	// precision as a JS number.
	JS_SetPropertyStr(ctx, object, "id", JS_NewString(
		ctx,
		QString::number(host->peerId).toUtf8().constData()));
	JS_SetPropertyStr(ctx, object, "name", JS_NewString(
		ctx,
		peer->name().toUtf8().constData()));
	JS_SetPropertyStr(ctx, object, "isUser", JS_NewBool(ctx, peer->isUser()));
	JS_SetPropertyStr(ctx, object, "isChat", JS_NewBool(ctx, peer->isChat()));
	JS_SetPropertyStr(
		ctx,
		object,
		"isChannel",
		JS_NewBool(ctx, peer->isChannel()));
	return object;
}

JSValue TgSend(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv) {
	const auto host = GetHost(ctx);
	if (!host || !host->session || !host->peerId || argc < 1) {
		return JS_FALSE;
	}
	const auto raw = JS_ToCString(ctx, argv[0]);
	if (!raw) {
		return JS_FALSE;
	}
	const auto text = QString::fromUtf8(raw);
	JS_FreeCString(ctx, raw);
	if (text.isEmpty()) {
		return JS_FALSE;
	}
	const auto history = host->session->data().history(PeerId(host->peerId));
	auto message = Api::MessageToSend(Api::SendAction(history));
	message.textWithTags.text = text;
	message.action.clearDraft = false; // Don't wipe the user's typed draft.
	history->session().api().sendMessage(std::move(message));
	return JS_TRUE;
}

[[nodiscard]] const char *MediaType(HistoryItem *item) {
	const auto media = item->media();
	if (!media) {
		return "none";
	} else if (media->photo()) {
		return "photo";
	} else if (media->document()) {
		return "document";
	}
	return "other";
}

JSValue TgHistory(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv) {
	auto array = JS_NewArray(ctx);
	const auto host = GetHost(ctx);
	if (!host || !host->session || !host->peerId) {
		return array;
	}
	auto limit = 50;
	if (argc >= 1) {
		JS_ToInt32(ctx, &limit, argv[0]);
	}
	const auto history = host->session->data().history(PeerId(host->peerId));
	auto items = std::vector<not_null<HistoryItem*>>();
	for (const auto &block : history->blocks) {
		for (const auto &view : block->messages) {
			items.push_back(view->data());
		}
	}
	const auto total = int(items.size());
	const auto start = (limit > 0 && total > limit) ? (total - limit) : 0;
	auto index = uint32_t(0);
	for (auto i = start; i != total; ++i) {
		const auto item = items[i];
		auto object = JS_NewObject(ctx);
		JS_SetPropertyStr(ctx, object, "id",
			JS_NewInt64(ctx, int64_t(item->id.bare)));
		JS_SetPropertyStr(ctx, object, "text", JS_NewString(
			ctx,
			item->originalText().text.toUtf8().constData()));
		JS_SetPropertyStr(ctx, object, "author", JS_NewString(
			ctx,
			item->from()->name().toUtf8().constData()));
		JS_SetPropertyStr(ctx, object, "out", JS_NewBool(ctx, item->out()));
		JS_SetPropertyStr(ctx, object, "date",
			JS_NewInt64(ctx, int64_t(item->date())));
		JS_SetPropertyStr(ctx, object, "media",
			JS_NewString(ctx, MediaType(item)));
		JS_SetPropertyUint32(ctx, array, index++, object);
	}
	return array;
}

JSValue TgDownload(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv) {
	const auto host = GetHost(ctx);
	if (!host || !host->session || !host->peerId || argc < 1) {
		return JS_NULL;
	}
	auto id = int64_t(0);
	if (JS_ToInt64(ctx, &id, argv[0]) < 0 || !id) {
		return JS_NULL;
	}
	const auto item = host->session->data().message(
		PeerId(host->peerId),
		MsgId(id));
	if (!item) {
		return JS_NULL;
	}
	const auto media = item->media();
	if (!media) {
		return JS_NULL;
	}
	const auto origin = Data::FileOrigin(item->fullId());
	const auto dir = cWorkingDir() + u"scripts/downloads/"_q;
	QDir().mkpath(dir);
	if (const auto document = media->document()) {
		auto name = document->filename();
		name.replace(QChar('/'), QChar('_'));
		name.replace(QChar('\\'), QChar('_'));
		if (name.isEmpty()) {
			name = u"doc_"_q + QString::number(id);
		}
		const auto path = dir + name;
		document->save(origin, path);
		return JS_NewString(ctx, path.toUtf8().constData());
	} else if (const auto photo = media->photo()) {
		const auto path = dir
			+ u"photo_"_q
			+ QString::number(id)
			+ u".jpg"_q;
		auto view = photo->createMediaView();
		view->wanted(Data::PhotoSize::Large, origin);
		if (view->loaded()) {
			view->saveToFile(path);
		} else {
			host->pendingPhotoSaves.push_back({ std::move(view), path });
		}
		return JS_NewString(ctx, path.toUtf8().constData());
	}
	return JS_NULL;
}

JSValue TgMarkRead(JSContext *ctx, JSValueConst, int, JSValueConst *) {
	const auto host = GetHost(ctx);
	if (!host || !host->session || !host->peerId) {
		return JS_FALSE;
	}
	const auto history = host->session->data().history(PeerId(host->peerId));
	host->session->data().histories().readInbox(history);
	return JS_TRUE;
}

JSValue TgNotify(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv) {
	if (argc >= 1) {
		const auto raw = JS_ToCString(ctx, argv[0]);
		if (raw) {
			Ui::Toast::Show(QString::fromUtf8(raw));
			JS_FreeCString(ctx, raw);
		}
	}
	return JS_UNDEFINED;
}

JSValue TgDelete(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv) {
	const auto host = GetHost(ctx);
	if (!host || !host->session || !host->peerId || argc < 1) {
		return JS_FALSE;
	}
	auto id = int64_t(0);
	if (JS_ToInt64(ctx, &id, argv[0]) < 0 || !id) {
		return JS_FALSE;
	}
	const auto item = host->session->data().message(
		PeerId(host->peerId),
		MsgId(id));
	if (!item) {
		return JS_FALSE;
	}
	auto revoke = true;
	if (argc >= 2) {
		revoke = (JS_ToBool(ctx, argv[1]) == 1);
	}
	host->session->data().histories().deleteMessages(
		{ item->fullId() },
		revoke);
	return JS_TRUE;
}

JSValue TgChats(JSContext *ctx, JSValueConst, int, JSValueConst *) {
	auto array = JS_NewArray(ctx);
	const auto host = GetHost(ctx);
	if (!host || !host->session) {
		return array;
	}
	const auto list = host->session->data().chatsList();
	auto index = uint32_t(0);
	for (const auto &row : list->indexed()->all()) {
		const auto history = row->history();
		if (!history) {
			continue;
		}
		const auto peer = history->peer;
		auto object = JS_NewObject(ctx);
		JS_SetPropertyStr(ctx, object, "id", JS_NewString(
			ctx,
			QString::number(peer->id.value).toUtf8().constData()));
		JS_SetPropertyStr(ctx, object, "name", JS_NewString(
			ctx,
			peer->name().toUtf8().constData()));
		JS_SetPropertyStr(ctx, object, "unread",
			JS_NewInt32(ctx, history->unreadCount()));
		JS_SetPropertyUint32(ctx, array, index++, object);
	}
	return array;
}

} // namespace

void InstallHost(JSContext *context, HostContext *host) {
	if (!context) {
		return;
	}
	JS_SetContextOpaque(context, host);
	auto global = JS_GetGlobalObject(context);
	auto tg = JS_NewObject(context);
	JS_SetPropertyStr(
		context,
		tg,
		"log",
		JS_NewCFunction(context, TgLog, "log", 1));
	JS_SetPropertyStr(
		context,
		tg,
		"send",
		JS_NewCFunction(context, TgSend, "send", 1));
	JS_SetPropertyStr(
		context,
		tg,
		"currentChat",
		JS_NewCFunction(context, TgCurrentChat, "currentChat", 0));
	JS_SetPropertyStr(
		context,
		tg,
		"history",
		JS_NewCFunction(context, TgHistory, "history", 1));
	JS_SetPropertyStr(
		context,
		tg,
		"download",
		JS_NewCFunction(context, TgDownload, "download", 1));
	JS_SetPropertyStr(
		context,
		tg,
		"markRead",
		JS_NewCFunction(context, TgMarkRead, "markRead", 0));
	JS_SetPropertyStr(
		context,
		tg,
		"notify",
		JS_NewCFunction(context, TgNotify, "notify", 1));
	JS_SetPropertyStr(
		context,
		tg,
		"del",
		JS_NewCFunction(context, TgDelete, "del", 2));
	JS_SetPropertyStr(
		context,
		tg,
		"chats",
		JS_NewCFunction(context, TgChats, "chats", 0));
	JS_SetPropertyStr(context, global, "tg", tg);
	JS_FreeValue(context, global);
}

void DrainPendingDownloads(HostContext *host) {
	if (!host) {
		return;
	}
	auto &pending = host->pendingPhotoSaves;
	for (auto i = pending.begin(); i != pending.end();) {
		if (i->media->loaded()) {
			i->media->saveToFile(i->path);
			i = pending.erase(i);
		} else {
			++i;
		}
	}
}

} // namespace Scripting
