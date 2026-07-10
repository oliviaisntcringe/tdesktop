/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include <memory>
#include <vector>

struct JSContext;

namespace Main {
class Session;
} // namespace Main

namespace Data {
class PhotoMedia;
} // namespace Data

namespace Scripting {

// A photo whose file we want on disk once its download finishes. The manager
// drains these on downloaderTaskFinished (see DrainPendingDownloads).
struct PendingPhotoSave {
	std::shared_ptr<Data::PhotoMedia> media;
	QString path;
};

// Ambient data the `tg.*` host functions read on each JS call. The manager
// updates `peerId` before running a chat's script, so the C-functions always
// act on the right chat.
struct HostContext {
	Main::Session *session = nullptr;
	uint64 peerId = 0;
	std::vector<PendingPhotoSave> pendingPhotoSaves;
};

// Installs the `tg` host object (log/send/currentChat/history/download) onto
// the context's global object and wires `host` as the context opaque.
void InstallHost(JSContext *context, HostContext *host);

// Writes any pending photos whose download has completed; call on each
// downloaderTaskFinished tick.
void DrainPendingDownloads(HostContext *host);

} // namespace Scripting
