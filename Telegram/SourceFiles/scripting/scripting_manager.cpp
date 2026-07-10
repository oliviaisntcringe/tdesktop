/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "scripting/scripting_manager.h"

#include "settings.h" // cWorkingDir
#include "core/application.h"
#include "core/core_settings.h"
#include "main/main_session.h"
#include "data/data_changes.h"
#include "data/data_session.h"
#include "data/data_peer.h"
#include "history/history.h"
#include "history/history_item.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace Scripting {

std::vector<QString> LibraryScripts() {
	auto result = std::vector<QString>();
	const auto dir = QDir(cWorkingDir() + u"scripts/lib"_q);
	const auto files = dir.entryList(
		QStringList{ u"*.js"_q },
		QDir::Files,
		QDir::Name);
	result.reserve(files.size());
	for (const auto &file : files) {
		auto name = file;
		if (name.endsWith(u".js"_q)) {
			name.chop(3);
		}
		result.push_back(name);
	}
	return result;
}

Manager::Manager(not_null<Main::Session*> session)
: _session(session) {
	_host.session = _session;
	InstallHost(_engine.context(), &_host);

	_session->changes().messageUpdates(
		Data::MessageUpdate::Flag::NewAdded
	) | rpl::on_next([=](const Data::MessageUpdate &update) {
		process(update.item);
	}, _lifetime);
}

void Manager::process(not_null<HistoryItem*> item) {
	if (item->out() || !item->isRegular()) {
		return;
	}
	const auto peer = item->history()->peer;
	const auto bareId = peer->id.value;
	if (Core::App().settings().chatScript(bareId).isEmpty()) {
		return;
	}
	auto message = QJsonObject();
	message.insert(u"text"_q, item->originalText().text);
	message.insert(u"author"_q, item->from()->name());
	message.insert(u"out"_q, item->out());
	message.insert(u"id"_q, double(item->id.bare));
	message.insert(u"date"_q, double(item->date()));
	auto event = QJsonObject();
	event.insert(u"type"_q, u"message"_q);
	event.insert(u"message"_q, message);
	runEvent(bareId, QString::fromUtf8(
		QJsonDocument(event).toJson(QJsonDocument::Compact)));
}

QString Manager::runEvent(uint64 peerId, const QString &eventJson) {
	const auto name = Core::App().settings().chatScript(peerId);
	if (name.isEmpty()) {
		return readState(peerId);
	}
	const auto code = scriptCode(name);
	if (code.isEmpty()) {
		return readState(peerId);
	}
	_host.peerId = peerId;
	const auto updated = _engine.run(code, eventJson, readState(peerId));
	writeState(peerId, updated);
	_updates.fire({});
	return updated;
}

QString Manager::libraryPath(const QString &name) const {
	return cWorkingDir() + u"scripts/lib/"_q + name + u".js"_q;
}

std::vector<QString> Manager::libraryList() const {
	return LibraryScripts();
}

QString Manager::scriptCode(const QString &name) const {
	auto file = QFile(libraryPath(name));
	if (!file.open(QIODevice::ReadOnly)) {
		return QString();
	}
	return QString::fromUtf8(file.readAll());
}

void Manager::saveScript(const QString &name, const QString &code) {
	QDir().mkpath(cWorkingDir() + u"scripts/lib"_q);
	auto file = QFile(libraryPath(name));
	if (file.open(QIODevice::WriteOnly)) {
		file.write(code.toUtf8());
	}
}

void Manager::deleteScript(const QString &name) {
	QFile::remove(libraryPath(name));
}

QString Manager::statePath(uint64 peerId) const {
	return cWorkingDir()
		+ u"scripts/state_"_q
		+ QString::number(peerId)
		+ u".json"_q;
}

QString Manager::readState(uint64 peerId) const {
	auto file = QFile(statePath(peerId));
	if (!file.open(QIODevice::ReadOnly)) {
		return QString();
	}
	return QString::fromUtf8(file.readAll());
}

void Manager::writeState(uint64 peerId, const QString &json) {
	QDir().mkpath(cWorkingDir() + u"scripts"_q);
	auto file = QFile(statePath(peerId));
	if (file.open(QIODevice::WriteOnly)) {
		file.write(json.toUtf8());
	}
}

QString Manager::state(uint64 peerId) {
	return readState(peerId);
}

rpl::producer<> Manager::updates() const {
	return _updates.events();
}

} // namespace Scripting
