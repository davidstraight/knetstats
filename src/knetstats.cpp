/***************************************************************************
 *   Copyright (C) 2004 by Hugo Parente Lima                               *
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

#include "knetstats.h"
#include "knetstatsview.h"
#include "configure.h"
#include "interface.h"

// Qt includes
#include <qstringlist.h>
#include <qdir.h>
#include <qtimer.h>
// KDE includes
#include <kconfig.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <kapplication.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kstdaccel.h>
#include <kstdaction.h>
#include <khelpmenu.h>
#include <kaboutapplication.h>
#include <kpopupmenu.h>
#include <kiconloader.h>
#include <kpushbutton.h>

KNetStats::KNetStats() : QWidget(0, "knetstats"), mConfigure(0), mBoot(true) {
	setupUi();
	int count;
	detectNewInterfaces(&count);
	if (!count && numInterfaces()) // caos!!! All interfaces are invisible! turn all visible...
		for(InterfaceMap::Iterator i = mInterfaces.begin(); i != mInterfaces.end(); ++i)
			i.data()->setVisible(true);


	mBoot = false;
	QTimer* timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(detectNewInterfaces()));
	timer->start(IF_DETECTION_INTERVAL);
}

KNetStats::~KNetStats() {
	for (InterfaceMap::Iterator it = mInterfaces.begin(); it != mInterfaces.end(); ++it)
		delete it.data();
}


void KNetStats::setupUi() {
	setIcon(kapp->icon());
	// Cria o contextMenu
	mActionCollection = new KActionCollection(this);
	mContextMenu = new KPopupMenu(this);
	mContextMenu->insertTitle( kapp->miniIcon(), kapp->caption() );
	KAction* configure = KStdAction::preferences(this, SLOT(configure()), mActionCollection, "configure");
	configure->plug(mContextMenu);
	mContextMenu->insertSeparator();
	KHelpMenu* helpmenu = new KHelpMenu(this, QString::null, false);
	mContextMenu->insertItem( i18n("&Help"), helpmenu->menu() );

	mContextMenu->insertSeparator();
	KAction* quitAction = KStdAction::quit(kapp, SLOT(quit()), mActionCollection);
	quitAction->plug(mContextMenu);

	connect(helpmenu, SIGNAL(showAboutApplication()), this, SLOT(about()));
}

QStringList KNetStats::scanInterfaces() {
	QDir dir("/sys/class/net");
	if (!dir.exists()) {
		KMessageBox::error(0, i18n("You need kernel 2.6.x with support to the /sys filesystem."));
		return QStringList();
	} else {
		QStringList list = dir.entryList(QDir::Dirs);
		list.pop_front(); // removes "." and ".." entries
		list.pop_front();
		return list;
	}
}

bool KNetStats::configure() {
	if (mConfigure)
		mConfigure->show();
	else {
/*		if (!mInterfaces.size()) {
			KMessageBox::error(this, i18n("You do not have any network interface.\nKNetStats will quit now."));
			return false;
		}*/

		mConfigure = new Configure(this, mInterfaces);
		connect(mConfigure->mOk, SIGNAL(clicked()), this, SLOT(configOk()));
		connect(mConfigure->mApply, SIGNAL(clicked()), this, SLOT(configApply()));
		connect(mConfigure->mCancel, SIGNAL(clicked()), this, SLOT(configCancel()));
		mConfigure->show();
	}
	return true;
}

void KNetStats::configOk() {
	if (mConfigure->canSaveConfig()) {
		saveConfig( mConfigure->options() );
		delete mConfigure;
		mConfigure = 0;
	}
}

void KNetStats::configApply() {
	if (mConfigure->canSaveConfig())
		saveConfig( mConfigure->options() );
}

void KNetStats::configCancel() {
	delete mConfigure;
	mConfigure = 0;
}

void KNetStats::saveConfig(const OptionsMap& options) {
	KConfig* cfg = kapp->config();

	for(OptionsMap::ConstIterator i = options.begin(); i != options.end(); ++i) {
		KConfigGroupSaver groupSaver(cfg, i.key());
		const KNetStatsView::Options& opt = i.data();
		// general
		cfg->writeEntry("UpdateInterval", opt.mUpdateInterval);
		cfg->writeEntry("ViewMode", opt.mViewMode);
		cfg->writeEntry("Monitoring", opt.mMonitoring);
		// txt view
		cfg->writeEntry("TxtFont", opt.mTxtFont);
		cfg->writeEntry("TxtUplColor", opt.mTxtUplColor);
		cfg->writeEntry("TxtDldColor", opt.mTxtDldColor);
		// IconView
		cfg->writeEntry("Theme", opt.mTheme);
		// Graphic view
		cfg->writeEntry("ChartUplColor", opt.mChartUplColor);
		cfg->writeEntry("ChartDldColor", opt.mChartDldColor);
		cfg->writeEntry("ChartBgColor", opt.mChartBgColor);
		cfg->writeEntry("ChartUseTransparentBackground", opt.mChartTransparentBackground);
	}

	for (InterfaceMap::ConstIterator i = mInterfaces.begin(); i != mInterfaces.end(); ++i)
		i.data()->update();
}

void KNetStats::about() {
	KAboutApplication dlg(this);
	dlg.exec();
}


void KNetStats::detectNewInterfaces(int* count_ptr) {
	int count = 0;
	const QStringList& interfaces = KNetStats::scanInterfaces();
	QStringList::ConstIterator i = interfaces.begin();
	for(; i != interfaces.end(); ++i) {
		InterfaceMap::Iterator elem = mInterfaces.find(*i);
		if (elem == mInterfaces.end())
			if (createInterface(*i))
				count++;
	}
	if (count && count_ptr)
		*count_ptr = count;
}

bool KNetStats::createInterface(const QString& name) {
	// Cria a interface e adiciona em mInterfaces.
	Interface* interface = new Interface(this, name);
	mInterfaces.insert(name, interface);
	if (!mBoot)
		interface->say(i18n("New interface detected: %1").arg(name));
	return interface->isVisible();
}

#include "knetstats.moc"
