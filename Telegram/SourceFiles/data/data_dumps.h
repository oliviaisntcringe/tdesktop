/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

class PeerData;

namespace Window {
class SessionController;
} // namespace Window

namespace Dumps {

struct Entry {
	QString file;
	QString title;
	uint64 peerId = 0;
	TimeId date = 0;
	int count = 0;
};

[[nodiscard]] QString Folder();
[[nodiscard]] std::vector<Entry> List();
void Remove(const Entry &entry);
void ExportCsv(const Entry &entry);

void Create(
	not_null<PeerData*> peer,
	Fn<void()> done = nullptr);

} // namespace Dumps
