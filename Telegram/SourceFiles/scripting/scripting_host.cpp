/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "scripting/scripting_host.h"

#include "main/main_session.h"
#include "apiwrap.h"
#include "api/api_common.h"
#include "data/data_session.h"
#include "data/data_peer.h"
#include "history/history.h"
#include "base/debug_log.h"

#include <quickjs.h>

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
	JS_SetPropertyStr(context, global, "tg", tg);
	JS_FreeValue(context, global);
}

} // namespace Scripting
