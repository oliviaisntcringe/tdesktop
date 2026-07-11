/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "settings/sections/settings_appearance.h"

#include "core/application.h"
#include "core/core_settings.h"
#include "window/themes/window_theme.h"
#include "settings/settings_common.h"
#include "ui/vertical_list.h"
#include "ui/wrap/vertical_layout.h"
#include "ui/widgets/buttons.h"
#include "styles/style_settings.h"

namespace Settings {

Type AppearanceId() {
	return Appearance::Id();
}

Appearance::Appearance(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> Appearance::title() {
	return rpl::single(u"Appearance"_q);
}

void Appearance::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);

	Ui::AddSkip(content);
	Ui::AddSubsectionTitle(content, rpl::single(u"Accent color"_q));

	struct Preset {
		QString name;
		QColor color;
	};
	const auto presets = std::vector<Preset>{
		{ u"Amber"_q, QColor(0xff, 0xb0, 0x00) },
		{ u"Green"_q, QColor(0x33, 0xff, 0x66) },
		{ u"Cyan"_q, QColor(0x33, 0xff, 0xff) },
		{ u"Magenta"_q, QColor(0xff, 0x66, 0xcc) },
		{ u"Red"_q, QColor(0xff, 0x55, 0x44) },
		{ u"White"_q, QColor(0xf0, 0xf0, 0xf0) },
	};
	for (const auto &preset : presets) {
		const auto color = preset.color;
		const auto button = AddButtonWithIcon(
			content,
			rpl::single(preset.name),
			st::settingsButton);
		button->setClickedCallback([=] {
			Window::Theme::ApplyAccentColor(color);
			Core::App().settings().setAccentColor(color);
			Core::App().saveSettingsDelayed();
		});
	}

	Ui::AddSkip(content);
	Ui::AddSubsectionTitle(content, rpl::single(u"Ghost mode"_q));

	const auto addGhost = [&](
			const QString &label,
			bool current,
			Fn<void(bool)> apply) {
		const auto button = AddButtonWithIcon(
			content,
			rpl::single(label),
			st::settingsButton);
		button->toggleOn(rpl::single(current));
		button->toggledChanges(
		) | rpl::on_next([=](bool value) {
			apply(value);
			Core::App().saveSettingsDelayed();
		}, button->lifetime());
	};
	addGhost(
		u"Don't send read receipts"_q,
		Core::App().settings().ghostRead(),
		[](bool v) { Core::App().settings().setGhostRead(v); });
	addGhost(
		u"Hide \"typing…\" status"_q,
		Core::App().settings().ghostTyping(),
		[](bool v) { Core::App().settings().setGhostTyping(v); });
	addGhost(
		u"Appear offline"_q,
		Core::App().settings().ghostOnline(),
		[](bool v) { Core::App().settings().setGhostOnline(v); });

	Ui::AddSkip(content);

	Ui::ResizeFitChild(this, content);
}

} // namespace Settings
