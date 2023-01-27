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

#include "configure.h"
#include "knetstats.h"

// Qt includes
#include <qstringlist.h>
#include <qlistbox.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qwidgetstack.h>
#include <qpopupmenu.h>
// Kde includes
#include <kapplication.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kconfig.h>
#include <kfontrequester.h>
#include <knuminput.h>
#include <kcolorbutton.h>
#include <kcombobox.h>
#include <kmessagebox.h>
#include <kpushbutton.h>


Configure::Configure(KNetStats* parent, const InterfaceMap& ifs) : ConfigureBase(parent) {
	// Load configuration
	KIconLoader* loader = kapp->iconLoader();

	QPixmap iconPCI = loader->loadIcon("icon_pci.png", KIcon::Small, 16);

	// Clone the configuration.
	for (InterfaceMap::ConstIterator it = ifs.begin(); it != ifs.end(); ++it) {
		mInterfaces->insertItem(iconPCI, it.key());
		mConfig[it.key()] = it.data()->options();

//		parent->readInterfaceOptions(*it, &mConfig[*it]);
	}

	mInterfaces->setCurrentItem(0);
	changeInterface(mInterfaces->selectedItem());

	connect(mInterfaces, SIGNAL(selectionChanged(QListBoxItem*)), this, SLOT(changeInterface(QListBoxItem*)));
	connect(mTheme, SIGNAL(activated(int)), this, SLOT(changeTheme(int)));
	//connect(mInterfaces, SIGNAL(contextMenuRequested(QListBoxItem*, const QPoint&)), this, SLOT(showInterfaceContextMenu(QListBoxItem*, const QPoint&)));
}

void Configure::changeInterface(QListBoxItem* item) {
	QString interface = item->text();

	if (!mCurrentItem.isEmpty())
	{
		// Salvas as modificações passadas
		KNetStatsView::Options& oldview = mConfig[mCurrentItem];
		// general options
		oldview.mMonitoring = mMonitoring->isChecked();
		oldview.mUpdateInterval = mUpdateInterval->value();
		oldview.mViewMode = (KNetStatsView::Mode) mViewMode->currentItem();
		// txt view options
		oldview.mTxtUplColor = mTxtUplColor->color();
		oldview.mTxtDldColor = mTxtDldColor->color();
		oldview.mTxtFont = mTxtFont->font();
		// icon view
		oldview.mTheme = mTheme->currentItem();
		// chart view
		oldview.mChartUplColor = mChartUplColor->color();
		oldview.mChartDldColor = mChartDldColor->color();
		oldview.mChartBgColor = mChartBgColor->color();
		oldview.mChartTransparentBackground = mChartTransparentBackground->isChecked();
	}

	if (interface == mCurrentItem)
		return;
	// Carrega as opt. da nova interface
	KNetStatsView::Options& view = mConfig[interface];
	// general
	mMonitoring->setChecked(view.mMonitoring);
	mUpdateInterval->setValue(view.mUpdateInterval);
	mViewMode->setCurrentItem(view.mViewMode);
	mWdgStack->raiseWidget(view.mViewMode);
	// txt options
	mTxtUplColor->setColor(view.mTxtUplColor);
	mTxtDldColor->setColor(view.mTxtDldColor);
	mTxtFont->setFont( view.mTxtFont );
	// icon options
	mTheme->setCurrentItem(view.mTheme);
	changeTheme(view.mTheme);
	// chart options
	mChartUplColor->setColor(view.mChartUplColor);
	mChartDldColor->setColor(view.mChartDldColor);
	mChartBgColor->setColor(view.mChartBgColor);
	mChartTransparentBackground->setChecked(view.mChartTransparentBackground);

	mCurrentItem = interface;
}

bool Configure::canSaveConfig()
{
	// update the options
	changeInterface(mInterfaces->item( mInterfaces->currentItem() ));

	bool ok = false;
	for(OptionsMap::ConstIterator i = mConfig.begin(); i != mConfig.end(); ++i)
		if (i.data().mMonitoring) {
			ok = true;
			break;
		}

	if (!ok)
		KMessageBox::error(this, i18n("You need to select at least one interface to monitor."));
	return ok;
}

void Configure::changeTheme(int theme)
{
	KIconLoader* loader = kapp->iconLoader();
	mIconError->setPixmap(loader->loadIcon("theme"+QString::number(theme)+"_error.png",
						  KIcon::Panel, ICONSIZE));
	mIconNone->setPixmap(loader->loadIcon("theme"+QString::number(theme)+"_none.png",
						 KIcon::Panel, ICONSIZE));
	mIconTx->setPixmap(loader->loadIcon("theme"+QString::number(theme)+"_tx.png",
					   KIcon::Panel, ICONSIZE));
	mIconRx->setPixmap(loader->loadIcon("theme"+QString::number(theme)+"_rx.png",
					   KIcon::Panel, ICONSIZE));
	mIconBoth->setPixmap(loader->loadIcon("theme"+QString::number(theme)+"_both.png",
						 KIcon::Panel, ICONSIZE));
}
/*
void Configure::showInterfaceContextMenu(QListBoxItem* item, const QPoint& point) {
	if (!item && mConfig.size() == 1)
		return;
	QPixmap icon = kapp->iconLoader()->loadIcon("editdelete", KIcon::Small, 16);
	QPopupMenu* menu = new QPopupMenu(this);
	menu->insertItem(icon, i18n("Renomve Interface"), this, SLOT(removeInterface()));
	menu->exec(point);
}

void Configure::removeInterface() {
	mConfig.erase(mInterfaces->currentText());
	mInterfaces->removeItem(mInterfaces->currentItem());
}
*/
#include "configure.moc"
