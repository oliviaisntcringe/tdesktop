/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "settings/sections/settings_dumps.h"

#include "data/data_dumps.h"
#include "core/file_utilities.h"
#include "settings/settings_common.h"
#include "ui/vertical_list.h"
#include "ui/wrap/vertical_layout.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/popup_menu.h"
#include "ui/widgets/labels.h"
#include "base/unixtime.h"
#include "base/unique_qptr.h"
#include "lang/lang_keys.h"
#include "styles/style_settings.h"
#include "styles/style_widgets.h"
#include "styles/style_menu_icons.h"

namespace Settings {

Type DumpsId() {
	return Dumps::Id();
}

Dumps::Dumps(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> Dumps::title() {
	return rpl::single(u"Dumps"_q);
}

void Dumps::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);

	Ui::AddSkip(content);
	Ui::AddSubsectionTitle(content, rpl::single(u"Saved dumps"_q));

	_list = content->add(object_ptr<Ui::VerticalLayout>(content));
	refresh();

	Ui::ResizeFitChild(this, content);
}

void Dumps::refresh() {
	const auto list = _list;
	list->clear();
	const auto entries = ::Dumps::List();
	if (entries.empty()) {
		Ui::AddDividerText(
			list,
			rpl::single(u"No dumps yet. Right-click a chat and choose "
				"\"Dump dialog\" to export it here."_q));
		return;
	}
	for (const auto &entry : entries) {
		const auto when = base::unixtime::parse(entry.date)
			.toString(u"yyyy-MM-dd hh:mm"_q);
		const auto label = u"%1 · %2 msgs"_q.arg(when).arg(entry.count);
		const auto button = AddButtonWithLabel(
			list,
			rpl::single(entry.title.isEmpty()
				? u"(unknown chat)"_q
				: entry.title),
			rpl::single(label),
			st::settingsButton,
			{ &st::menuIconExport });
		const auto menu = button->lifetime().make_state<
			base::unique_qptr<Ui::PopupMenu>>();
		button->setClickedCallback([=] {
			*menu = base::make_unique_q<Ui::PopupMenu>(
				button,
				st::popupMenuWithIcons);
			(*menu)->addAction(u"Export CSV…"_q, [=] {
				::Dumps::ExportCsv(entry);
			}, &st::menuIconExport);
			(*menu)->addAction(u"Open file"_q, [=] {
				File::Launch(entry.file);
			}, &st::menuIconShowInFolder);
			(*menu)->addAction(u"Delete"_q, [=] {
				::Dumps::Remove(entry);
				crl::on_main(this, [=] { refresh(); });
			}, &st::menuIconDelete);
			(*menu)->popup(QCursor::pos());
		});
	}
}

} // namespace Settings
