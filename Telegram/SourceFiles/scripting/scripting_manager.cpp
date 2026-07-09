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
namespace {

// Built-in script for the counter proof (Phase 2). User scripts come later.
constexpr auto kCounterScript = R"(
state.count = (state.count || 0) + 1;
if (message) {
	state.last = message.text;
}
)";

[[nodiscard]] QString MessageJson(not_null<HistoryItem*> item) {
	auto object = QJsonObject();
	object.insert(u"text"_q, item->originalText().text);
	object.insert(u"out"_q, item->out());
	object.insert(u"id"_q, double(item->id.bare));
	return QString::fromUtf8(
		QJsonDocument(object).toJson(QJsonDocument::Compact));
}

} // namespace

Manager::Manager(not_null<Main::Session*> session)
: _session(session) {
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
	if (!Core::App().settings().scriptedPeer(bareId)) {
		return;
	}
	const auto updated = _engine.run(
		QString::fromUtf8(kCounterScript),
		MessageJson(item),
		readState(bareId));
	writeState(bareId, updated);
	_updates.fire({});
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
