/***************************************************************************
 *   Copyright (C) 2004-2006 by Hugo Parente Lima                          *
 *   hugo_pl@users.sourceforge.net                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/

#include "interface.h"
#include <kapplication.h>
#include <kapp.h>
#include <kconfig.h>

Interface::Interface(KNetStats* parent, const QString& name) : mName(name), mView(0), mParent(parent) {
	update();
}

void Interface::update() {
	bool defaultVisibility = !(mName == "lo" || mName == "sit0");

	KConfig* cfg = kapp->config();
	KConfigGroupSaver groupSaver(cfg, mName);
	bool visible = cfg->readBoolEntry("Monitoring", defaultVisibility);
	if (!visible)
		setVisible(false);
	else if (visible && !mView)
		setVisible(true);
	else if (visible && mView)
		mView->updateViewOptions();
}

void Interface::setVisible(bool visible) {
	if (!visible) {
		delete mView;
		mView = 0;
	} else if (visible && !mView)
		mView = new KNetStatsView(mParent, mName);
}

KNetStatsView::Options Interface::options() {
	if (mView)
		return mView->options();
	else {
		KNetStatsView::Options opt;
		KNetStatsView::readOptions(mName, &opt, false);
		return opt;
	}
}

void Interface::say(const QString& message) {
	if (mView)
		mView->say(message);
}

