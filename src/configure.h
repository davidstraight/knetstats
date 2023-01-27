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

#ifndef CONFIGURE_H
#define CONFIGURE_H

#include "configurebase.h"
#include "interface.h"
#include <qstring.h>
#include <qmap.h>
#include <qfont.h>
#include <qcolor.h>
#include <qpixmap.h>
#include <qstringlist.h>

static const int ICONSIZE = 22;

class KNetStats;
class QListBoxItem;


typedef QMap<QString, KNetStatsView::Options> OptionsMap;


/**
*	Configure dialog
*/
class Configure : public ConfigureBase
{
	Q_OBJECT
public:
	Configure(KNetStats* parent, const InterfaceMap& ifs);

	const OptionsMap& currentConfig() const { return mConfig; }
	bool canSaveConfig();
	const OptionsMap& options() const { return mConfig; }
private:
	QString mCurrentItem;
	OptionsMap mConfig;

protected slots:
	void changeInterface(QListBoxItem* item);
	void changeTheme(int theme);
	//void showInterfaceContextMenu(QListBoxItem* item, const QPoint& point);
	//void removeInterface();
};

#endif
