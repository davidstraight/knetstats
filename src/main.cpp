/***************************************************************************
 *   Copyright (C) 2004-2008 by Hugo Parente Lima                          *
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <kapplication.h>
#include <klocale.h>
#include <kmessagebox.h>

#include "knetstats.h"

const char* programName = I18N_NOOP( "KNetStats" );

int main( int argc, char** argv )
{
	KAboutData aboutData( "knetstats", programName,
	                      "v1.6.2",									// version
						  I18N_NOOP( "A network device monitor." ),	// description
						  KAboutData::License_GPL,					// license
	                      "(C) 2004-2008 Hugo Parente Lima",		// copyright
						  0,
						  "http://knetstats.sourceforge.net",		// homepage
						  "hugo_pl@users.sourceforge.net");			// bug email address
	aboutData.setTranslator(I18N_NOOP("_: NAME OF TRANSLATORS"), I18N_NOOP("_: EMAIL OF TRANSLATORS"));
	aboutData.addAuthor( "Hugo Parente Lima", 0, "hugo_pl@users.sourceforge.net" );

	// Credits
	aboutData.addCredit( "Thomas Windheuser", I18N_NOOP("SCons help, Debian packages, etc."), 0);
	aboutData.addCredit( "KNemo", I18N_NOOP("Icon themes"), 0,
						"http://kde-apps.org/content/show.php?content=12956" );
	// translators
	aboutData.addCredit( "Ilyas Bakirov, Roberto Leandrini, Carlos Ortiz, Henrik Gebauer, Edward Romantsov, Wiktor Wandachowicz, Guillaume Savaton, Petar Toushkov, Liu Di", I18N_NOOP("KNetStats translation to other languages"), 0);

	KCmdLineArgs::init( argc, argv, &aboutData );
	KApplication::disableAutoDcopRegistration();

	KApplication app;
	KNetStats knetstats;
	if (!knetstats.numInterfaces()) {
		KMessageBox::error(0, i18n("You do not have any network interface.\nKNetStats will quit now."));
		return 1;
	}
	return app.exec();
}

