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
#ifndef STATISTICS_H
#define STATISTICS_H

#include "statisticsbase.h"
#include <qstring.h>

class KNetStatsView;

class Statistics : public StatisticsBase
{
	Q_OBJECT

public:
	Statistics( KNetStatsView* parent = 0, const char *name = 0 );

	/**
	*	Formats a numberic byte representation
	*	\param	num		The numeric representation
	*	\param	decimal	Decimal digits
	*	\param	bytesufix	Sufix for bytes
	*	\param	ksufix	Sufix for kilobytes
	*	\param	msufix	Sufix for megabytes
	*/
	static inline QString byteFormat( double num, const char* ksufix = " KB", const char* msufix = " MB");

	void show();
private:
	QTimer* mTimer;
	const QString& mInterface;
	KNetStatsView* mParent;
public slots:
	void accept();

private slots:
	void update();
};

QString Statistics::byteFormat( double num, const char* ksufix, const char* msufix ) {
	const double ONE_KB = 1024.0;
	const double ONE_MB = ONE_KB*ONE_KB;
	if ( num >= ONE_MB ) 	// MB
		return QString::number( num/(ONE_MB), 'f', 1 ) + msufix;
	else	// Kb
		return QString::number( num/ONE_KB, 'f', 1 ) + ksufix;
}

#endif
