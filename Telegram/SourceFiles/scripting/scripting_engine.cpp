/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "scripting/scripting_engine.h"

#include <quickjs.h>

namespace Scripting {

Engine::Engine()
: _runtime(JS_NewRuntime())
, _context(_runtime ? JS_NewContext(_runtime) : nullptr) {
}

Engine::~Engine() {
	if (_context) {
		JS_FreeContext(_context);
	}
	if (_runtime) {
		JS_FreeRuntime(_runtime);
	}
}

QString Engine::eval(const QString &code) {
	if (!_context) {
		return u"error: no context"_q;
	}
	const auto utf8 = code.toUtf8();
	auto value = JS_Eval(
		_context,
		utf8.constData(),
		size_t(utf8.size()),
		"<eval>",
		JS_EVAL_TYPE_GLOBAL);
	const auto cleanup = [&](JSValue v) {
		JS_FreeValue(_context, v);
	};
	if (JS_IsException(value)) {
		auto exception = JS_GetException(_context);
		const auto raw = JS_ToCString(_context, exception);
		auto text = QString::fromUtf8(raw ? raw : "exception");
		if (raw) {
			JS_FreeCString(_context, raw);
		}
		cleanup(exception);
		cleanup(value);
		return u"error: "_q + text;
	}
	const auto raw = JS_ToCString(_context, value);
	auto text = QString::fromUtf8(raw ? raw : "");
	if (raw) {
		JS_FreeCString(_context, raw);
	}
	cleanup(value);
	return text;
}

} // namespace Scripting
