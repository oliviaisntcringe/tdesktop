/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

struct JSContext;

namespace Main {
class Session;
} // namespace Main

namespace Scripting {

// Ambient data the `tg.*` host functions read on each JS call. The manager
// updates `peerId` before running a chat's script, so the C-functions always
// act on the right chat.
struct HostContext {
	Main::Session *session = nullptr;
	uint64 peerId = 0;
};

// Installs the `tg` host object (log/send/currentChat/…) onto the context's
// global object and wires `host` as the context opaque for the C-functions.
void InstallHost(JSContext *context, HostContext *host);

} // namespace Scripting
