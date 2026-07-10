/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "scripting/scripting_engine.h"
#include "scripting/scripting_host.h"

class HistoryItem;

namespace Main {
class Session;
} // namespace Main

namespace Scripting {

// Names of scripts in the library (files under scripts/lib/), filesystem-read.
[[nodiscard]] std::vector<QString> LibraryScripts();

class Manager final {
public:
	explicit Manager(not_null<Main::Session*> session);

	// Current persisted state (JSON) for a peer's attached script.
	[[nodiscard]] QString state(uint64 peerId);

	// Named script library (files under scripts/lib/).
	[[nodiscard]] std::vector<QString> libraryList() const;
	[[nodiscard]] QString scriptCode(const QString &name) const;
	void saveScript(const QString &name, const QString &code);
	void deleteScript(const QString &name);

	// Runs a chat's bound script for an event JSON; persists + returns state.
	QString runEvent(uint64 peerId, const QString &eventJson);

	// Fires whenever any chat's state changes.
	[[nodiscard]] rpl::producer<> updates() const;

	// Last uncaught error from a peer's script (empty if none / other peer).
	[[nodiscard]] QString lastError(uint64 peerId) const;

private:
	void process(not_null<HistoryItem*> item);
	void emitItemEvent(not_null<HistoryItem*> item, const QString &type);
	[[nodiscard]] QString libraryPath(const QString &name) const;
	[[nodiscard]] QString statePath(uint64 peerId) const;
	[[nodiscard]] QString readState(uint64 peerId) const;
	void writeState(uint64 peerId, const QString &json);

	const not_null<Main::Session*> _session;
	Engine _engine;
	HostContext _host;
	rpl::event_stream<> _updates;
	QString _lastError;
	uint64 _lastErrorPeer = 0;
	rpl::lifetime _lifetime;

};

} // namespace Scripting
