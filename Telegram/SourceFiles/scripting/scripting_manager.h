/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "scripting/scripting_engine.h"
#include "base/flat_map.h"

class HistoryItem;

namespace Main {
class Session;
} // namespace Main

namespace Scripting {

class Manager final {
public:
	explicit Manager(not_null<Main::Session*> session);

	// Current persisted state (JSON) for a peer's attached script.
	[[nodiscard]] QString state(uint64 peerId);

	// The user-editable JS script for a given chat (per-chat).
	[[nodiscard]] QString script(uint64 peerId);
	void setScript(uint64 peerId, const QString &text);

	// Fires whenever any attached script's state changes.
	[[nodiscard]] rpl::producer<> updates() const;

private:
	void process(not_null<HistoryItem*> item);
	[[nodiscard]] QString loadScript(uint64 peerId) const;
	[[nodiscard]] QString scriptPath(uint64 peerId) const;
	[[nodiscard]] QString statePath(uint64 peerId) const;
	[[nodiscard]] QString readState(uint64 peerId) const;
	void writeState(uint64 peerId, const QString &json);

	const not_null<Main::Session*> _session;
	Engine _engine;
	base::flat_map<uint64, QString> _scriptCache;
	rpl::event_stream<> _updates;
	rpl::lifetime _lifetime;

};

} // namespace Scripting
