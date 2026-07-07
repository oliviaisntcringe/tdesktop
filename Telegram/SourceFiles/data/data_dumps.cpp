/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "data/data_dumps.h"

#include "settings.h" // cWorkingDir
#include "apiwrap.h"
#include "core/application.h"
#include "core/file_utilities.h"
#include "data/data_peer.h"
#include "data/data_session.h"
#include "data/data_types.h"
#include "history/history.h"
#include "history/history_item.h"
#include "main/main_session.h"
#include "base/unixtime.h"
#include "ui/toast/toast.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace Dumps {
namespace {

constexpr auto kPerPage = 100;

[[nodiscard]] QString IndexPath() {
	return Folder() + u"index.json"_q;
}

[[nodiscard]] QString EscapeCsv(const QString &value) {
	if (value.contains('"')
		|| value.contains(',')
		|| value.contains('\n')
		|| value.contains('\r')) {
		auto escaped = value;
		escaped.replace('"', u"\"\""_q);
		return '"' + escaped + '"';
	}
	return value;
}

[[nodiscard]] QJsonArray ReadIndex() {
	auto file = QFile(IndexPath());
	if (!file.open(QIODevice::ReadOnly)) {
		return QJsonArray();
	}
	const auto parsed = QJsonDocument::fromJson(file.readAll());
	return parsed.isArray() ? parsed.array() : QJsonArray();
}

void WriteIndex(const QJsonArray &array) {
	QDir().mkpath(Folder());
	auto file = QFile(IndexPath());
	if (file.open(QIODevice::WriteOnly)) {
		file.write(QJsonDocument(array).toJson(QJsonDocument::Compact));
	}
}

[[nodiscard]] Entry EntryFromJson(const QJsonObject &object) {
	auto result = Entry();
	result.file = Folder() + object.value(u"file"_q).toString();
	result.title = object.value(u"title"_q).toString();
	result.peerId = object.value(u"peer"_q).toString().toULongLong();
	result.date = TimeId(object.value(u"date"_q).toInt());
	result.count = object.value(u"count"_q).toInt();
	return result;
}

} // namespace

QString Folder() {
	return cWorkingDir() + u"dumps/"_q;
}

std::vector<Entry> List() {
	const auto array = ReadIndex();
	auto result = std::vector<Entry>();
	result.reserve(array.size());
	for (const auto &value : array) {
		if (value.isObject()) {
			result.push_back(EntryFromJson(value.toObject()));
		}
	}
	ranges::sort(result, ranges::greater(), &Entry::date);
	return result;
}

void Remove(const Entry &entry) {
	const auto name = QFileInfo(entry.file).fileName();
	QFile::remove(entry.file);
	auto array = ReadIndex();
	for (auto i = array.begin(); i != array.end();) {
		if (i->toObject().value(u"file"_q).toString() == name) {
			i = array.erase(i);
		} else {
			++i;
		}
	}
	WriteIndex(array);
}

void ExportCsv(const Entry &entry) {
	const auto from = entry.file;
	const auto suggested = QFileInfo(from).fileName();
	FileDialog::GetWritePath(
		Core::App().getFileDialogParent(),
		u"Export dump to CSV"_q,
		u"CSV file (*.csv)"_q,
		suggested,
		[=](QString &&result) {
			if (result.isEmpty()) {
				return;
			}
			QFile::remove(result);
			if (QFile::copy(from, result)) {
				Ui::Toast::Show(u"Exported to CSV."_q);
			} else {
				Ui::Toast::Show(u"Could not export."_q);
			}
		});
}

void Create(not_null<PeerData*> peer, Fn<void()> done) {
	struct State {
		QStringList rows;
		MsgId offsetId = 0;
		Fn<void()> next;
	};
	const auto state = std::make_shared<State>();
	const auto weak = std::weak_ptr<State>(state);

	const auto finish = [peer, done](const std::shared_ptr<State> &state) {
		auto &rows = state->rows;
		std::reverse(rows.begin(), rows.end());
		if (rows.isEmpty()) {
			Ui::Toast::Show(u"Nothing to dump."_q);
			if (done) {
				done();
			}
			return;
		}
		const auto now = base::unixtime::now();
		const auto header = u"id,date,author,text"_q;
		const auto content = (header + '\n' + rows.join('\n') + '\n').toUtf8();
		const auto name = u"dump_%1_%2.csv"_q.arg(peer->id.value).arg(now);
		QDir().mkpath(Folder());
		auto file = QFile(Folder() + name);
		if (!file.open(QIODevice::WriteOnly)
			|| file.write(content) != content.size()) {
			Ui::Toast::Show(u"Could not write dump."_q);
			if (done) {
				done();
			}
			return;
		}
		file.close();

		auto array = ReadIndex();
		auto object = QJsonObject();
		object.insert(u"file"_q, name);
		object.insert(u"title"_q, peer->name());
		object.insert(u"peer"_q, QString::number(peer->id.value));
		object.insert(u"date"_q, int(now));
		object.insert(u"count"_q, int(rows.size()));
		array.push_back(object);
		WriteIndex(array);

		Ui::Toast::Show(
			u"Saved dump: %1 messages."_q.arg(rows.size()));
		if (done) {
			done();
		}
	};

	state->next = [=] {
		const auto strong = weak.lock();
		if (!strong) {
			return;
		}
		peer->session().api().request(MTPmessages_GetHistory(
			peer->input(),
			MTP_int(strong->offsetId),
			MTP_int(0), // offset_date
			MTP_int(0), // add_offset
			MTP_int(kPerPage),
			MTP_int(0), // max_id
			MTP_int(0), // min_id
			MTP_long(0) // hash
		)).done([peer, state = weak.lock(), finish](
				const MTPmessages_Messages &result) {
			auto &owner = peer->owner();
			const auto grab = [&](const auto &data)
					-> const QVector<MTPMessage>* {
				owner.processUsers(data.vusers());
				owner.processChats(data.vchats());
				return &data.vmessages().v;
			};
			const auto list = result.match([&](
					const MTPDmessages_messages &data) {
				return grab(data);
			}, [&](const MTPDmessages_messagesSlice &data) {
				return grab(data);
			}, [&](const MTPDmessages_channelMessages &data) {
				return grab(data);
			}, [&](const MTPDmessages_messagesNotModified &) {
				return (const QVector<MTPMessage>*)nullptr;
			});
			if (!list || list->isEmpty()) {
				finish(state);
				return;
			}
			owner.processMessages(*list, NewMessageType::Existing);
			auto minId = MsgId(0);
			for (const auto &message : *list) {
				const auto id = IdFromMessage(message);
				if (const auto item = owner.message(peer->id, id)) {
					if (!item->isService()) {
						const auto date = base::unixtime::parse(item->date())
							.toString(u"yyyy-MM-dd hh:mm:ss"_q);
						const auto author = item->from()->name();
						const auto text = item->originalText().text;
						state->rows.push_back(u"%1,%2,%3,%4"_q
							.arg(QString::number(id.bare))
							.arg(EscapeCsv(date))
							.arg(EscapeCsv(author))
							.arg(EscapeCsv(text)));
					}
				}
				if (!minId || id < minId) {
					minId = id;
				}
			}
			const auto reachedTop = (int(list->size()) < kPerPage)
				|| !minId
				|| (state->offsetId && minId >= state->offsetId);
			state->offsetId = minId;
			if (reachedTop) {
				finish(state);
			} else {
				state->next();
			}
		}).fail([state = weak.lock(), finish] {
			finish(state);
		}).send();
	};
	Ui::Toast::Show(u"Exporting dialog…"_q);
	state->next();
}

} // namespace Dumps
