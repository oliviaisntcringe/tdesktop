/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

struct JSRuntime;
struct JSContext;

namespace Scripting {

class Engine final {
public:
	Engine();
	~Engine();

	Engine(const Engine &) = delete;
	Engine &operator=(const Engine &) = delete;

	[[nodiscard]] QString eval(const QString &code);

private:
	JSRuntime *_runtime = nullptr;
	JSContext *_context = nullptr;

};

} // namespace Scripting
