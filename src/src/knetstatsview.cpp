/***************************************************************************
 *   Copyright (C) 2004-2005 by Hugo Parente Lima                               *
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
#include "knetstatsview.h"
#include "knetstats.h"

// KDE headers
#include <kapplication.h>
#include <kiconloader.h>
#include <kpopupmenu.h>
#include <kmessagebox.h>
#include <kaboutapplication.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kstandarddirs.h>

#include <kaction.h>
#include <kstdaccel.h>
#include <kstdaction.h>
#include <khelpmenu.h>
#include <kpassivepopup.h>

// Qt headers
#include <qtimer.h>
#include <qfile.h>
#include <qtooltip.h>
#include <qcursor.h>
#include <qevent.h>
#include <qfont.h>
#include <qfontmetrics.h>
#include <qpainter.h>

// C headers
#include <cstring>
#include <cstdio>

// Linux headers
extern "C" {
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
}

#include "configure.h"
#include "statistics.h"

extern const char* programName;

KNetStatsView::KNetStatsView(KNetStats* parent, const QString& interface)
: KSystemTray(parent, 0), mSysDevPath("/sys/class/net/"+interface+'/'), mInterface(interface) {
	mFdSock = 0;
	mFirstUpdate = true;
	mMaxSpeedAge = 0;
	mMaxSpeed = 0.0;
	mBRx = mBTx = mPRx = mPTx = 0;
	mConnected = mCarrier = true;
	mTotalBytesRx = mTotalBytesTx = mTotalPktRx = mTotalPktTx = 0;
	mSpeedBufferPtr = mSpeedHistoryPtr = 0;
	mStatistics = 0;

	readOptions(interface, &mOptions, true);
	resetBuffers();
	openFdSocket();
	memset(&mDevInfo, 0, sizeof(mDevInfo));
	strcpy(mDevInfo.ifr_name, mInterface.latin1());

	setTextFormat(Qt::PlainText);
	mContextMenu = parent->contextMenu();
	show();

	// Timer
	mTimer = new QTimer(this, "timer");
	connect(mTimer, SIGNAL(timeout()), this, SLOT(updateStats(void)));

	QToolTip::add(this, i18n("Monitoring %1").arg(mInterface));
	setup();
	mStatistics = new Statistics(this);
}

KNetStatsView::~KNetStatsView() {
	if (mFdSock != -1)
		shutdown(mFdSock, SHUT_RDWR);

}

void KNetStatsView::setup() {
	if (mOptions.mViewMode == Text)
		setFont(mOptions.mTxtFont);
	else if (mOptions.mViewMode == Icon) {
		// Load Icons
		KIconLoader* loader = kapp->iconLoader();
		mIconError = loader->loadIcon("theme"+QString::number(mOptions.mTheme)+"_error.png",
							KIcon::Panel, ICONSIZE);
		mIconNone = loader->loadIcon("theme"+QString::number(mOptions.mTheme)+"_none.png",
							KIcon::Panel, ICONSIZE);
		mIconTx = loader->loadIcon("theme"+QString::number(mOptions.mTheme)+"_tx.png",
						KIcon::Panel, ICONSIZE);
		mIconRx =loader->loadIcon("theme"+QString::number(mOptions.mTheme)+"_rx.png",
						KIcon::Panel, ICONSIZE);
		mIconBoth = loader->loadIcon("theme"+QString::number(mOptions.mTheme)+"_both.png",
							KIcon::Panel, ICONSIZE);
		mCurrentIcon = &mIconNone;
	}
	mTimer->start(mOptions.mUpdateInterval);
	updateStats();
	QWidget::update();
	mFirstUpdate = false;
}

void KNetStatsView::say(const QString& message) {
		KPassivePopup::message(programName, message, kapp->miniIcon(), this);
}

void KNetStatsView::updateViewOptions() {
	readOptions(mInterface, &mOptions);
	setup();
}

void KNetStatsView::updateStats() {
	FILE* flags_fp = fopen((mSysDevPath+"flags").latin1(), "r");
	bool currentStatus;
	if (!flags_fp)
		currentStatus = false;
	else {
		int flags;
		fscanf(flags_fp, "%Xu", &flags);
		fclose(flags_fp);
		currentStatus = IFF_UP & flags;
	}

	if (!currentStatus && mConnected) { // interface down...
		mConnected = false;
		resetBuffers();
		QWidget::update();
		say(i18n("%1 is inactive").arg(mInterface));
	} else if (currentStatus && !mConnected) {
		mConnected = true;
		say(i18n("%1 is active").arg(mInterface));
	}

	// kernel < 2.6.9 (I think) does not have carrier info, considering carrier ever on.
	FILE* carrier_fp = fopen((mSysDevPath+"carrier").latin1(), "r");
	char carrierFlag = '1';
	if (carrier_fp) {
		carrierFlag = fgetc(carrier_fp);
		fclose(carrier_fp);
	}

	if (!mConnected)
		return;
	if (carrierFlag == '0') { // carrier down
		if (mCarrier) {
			mCarrier = false;
			QWidget::update();
			say(i18n("%1 is disconnected").arg(mInterface));
		}
		return;
	} else if (!mCarrier) { // carrier up
		mCarrier = true;
		say(i18n("%1 is connected").arg(mInterface));
	}

	unsigned int brx = readInterfaceNumValue("rx_bytes");
	unsigned int btx = readInterfaceNumValue("tx_bytes");
	unsigned int prx = readInterfaceNumValue("rx_packets");
	unsigned int ptx = readInterfaceNumValue("tx_packets");


	if (!mFirstUpdate) { // a primeira velocidade sempre eh absurda, para evitar isso temos o mFirstUpdate
		if (++mSpeedBufferPtr >= SPEED_BUFFER_SIZE)
			mSpeedBufferPtr = 0;

		// Calcula as velocidades
		mSpeedBufferTx[mSpeedBufferPtr] = ((btx - mBTx)*(1000.0f/mOptions.mUpdateInterval));
		mSpeedBufferRx[mSpeedBufferPtr] = ((brx - mBRx)*(1000.0f/mOptions.mUpdateInterval));
		mSpeedBufferPTx[mSpeedBufferPtr] = ((ptx - mPTx)*(1000.0f/mOptions.mUpdateInterval));
		mSpeedBufferPRx[mSpeedBufferPtr] = ((prx - mPRx)*(1000.0f/mOptions.mUpdateInterval));

		if (++mSpeedHistoryPtr >= HISTORY_SIZE)
			mSpeedHistoryPtr = 0;
		mSpeedHistoryRx[mSpeedHistoryPtr] = calcSpeed(mSpeedBufferRx);
		mSpeedHistoryTx[mSpeedHistoryPtr] = calcSpeed(mSpeedBufferTx);

		mMaxSpeedAge--;

		if (mSpeedHistoryTx[mSpeedHistoryPtr] > mMaxSpeed) {
			mMaxSpeed = mSpeedHistoryTx[mSpeedHistoryPtr];
			mMaxSpeedAge = HISTORY_SIZE;
		}
		if (mSpeedHistoryRx[mSpeedHistoryPtr] > mMaxSpeed)  {
			mMaxSpeed = mSpeedHistoryRx[mSpeedHistoryPtr];
			mMaxSpeedAge = HISTORY_SIZE;
		}
		if (mMaxSpeedAge < 1)
			calcMaxSpeed();
	}

	if (mOptions.mViewMode == Icon) {
		QPixmap* newIcon;
		if (brx == mBRx) {
			if (btx == mBTx )
				newIcon = &mIconNone;
			else
				newIcon = &mIconTx;
		} else {
			if (btx == mBTx )
				newIcon = &mIconRx;
			else
				newIcon = &mIconBoth;
		}

		if (newIcon != mCurrentIcon) {
			mCurrentIcon = newIcon;
			QWidget::update();
		}
	} else if (mOptions.mViewMode == Graphic || (btx != mBTx && brx != mBRx))
		QWidget::update();

	// Update stats
	mTotalBytesRx += brx - mBRx;
	mTotalBytesTx += btx - mBTx;
	mTotalPktRx += prx - mPRx;
	mTotalPktTx += ptx - mPTx;

	mBRx = brx;
	mBTx = btx;
	mPRx = prx;
	mPTx = ptx;
}

unsigned long KNetStatsView::readInterfaceNumValue(const char* name) {
	// stdio functions appear to be more fast than QFile?
	FILE* fp = fopen((mSysDevPath+"statistics/"+name).latin1(), "r");
	long retval;
	fscanf(fp, "%lu", &retval);
	fclose(fp);
	return retval;
}

QString KNetStatsView::readInterfaceStringValue(const char* name, int maxlength) {
	QFile macFile(mSysDevPath+name);
	macFile.open(IO_ReadOnly);
	QString value;
	macFile.readLine(value, maxlength);
	return value;
}

void KNetStatsView::resetBuffers() {
	memset(mSpeedHistoryRx, 0, sizeof(double)*HISTORY_SIZE);
	memset(mSpeedHistoryTx, 0, sizeof(double)*HISTORY_SIZE);
	memset(mSpeedBufferRx, 0, sizeof(double)*SPEED_BUFFER_SIZE);
	memset(mSpeedBufferTx, 0, sizeof(double)*SPEED_BUFFER_SIZE);
	memset(mSpeedBufferPRx, 0, sizeof(double)*SPEED_BUFFER_SIZE);
	memset(mSpeedBufferPTx, 0, sizeof(double)*SPEED_BUFFER_SIZE);
}

void KNetStatsView::paintEvent( QPaintEvent* ) {
	QPainter paint(this);
	switch(mOptions.mViewMode) {
		case Icon:
			if (!mCarrier || !mConnected)
				mCurrentIcon = &mIconError;
			paint.drawPixmap(0, 0, *mCurrentIcon);
			break;
		case Text:
			drawText(paint);
			break;
		case Graphic:
			drawGraphic(paint);
			break;
	}
}

void KNetStatsView::drawText(QPainter& paint) {
	if (!mCarrier || !mConnected) {
		paint.drawText(rect(), Qt::AlignCenter, "?");
	} else {
		paint.setFont( mOptions.mTxtFont );
		paint.setPen( mOptions.mTxtUplColor );
		paint.drawText( rect(), Qt::AlignTop, Statistics::byteFormat(byteSpeedTx(), "KB", "MB"));
		paint.setPen( mOptions.mTxtDldColor );
		paint.drawText( rect(), Qt::AlignBottom, Statistics::byteFormat(byteSpeedRx(), "KB", "MB"));
	}
}

void KNetStatsView::drawGraphic(QPainter& paint) {
	if (!mCarrier || !mConnected) {
		paint.drawText(rect(), Qt::AlignCenter, "X");
		return;
	}

	QSize size = this->size();

	if (!mOptions.mChartTransparentBackground)
		paint.fillRect(0, 0, size.width(), size.height(), mOptions.mChartBgColor);

	const int HEIGHT = size.height()-1;

	//	qDebug("MaxSpeed: %d, age: %d", int(mMaxSpeed), mMaxSpeedAge);
	int lastX;
	int lastRxY = HEIGHT - int(HEIGHT * (mSpeedHistoryRx[mSpeedHistoryPtr]/mMaxSpeed));
	int lastTxY = HEIGHT - int(HEIGHT * (mSpeedHistoryTx[mSpeedHistoryPtr]/mMaxSpeed));
	int x = lastX = size.width();
	int count = 0;
	for (int i = mSpeedHistoryPtr; count < width(); i--) {
		if (i < 0)
			i = HISTORY_SIZE-1;

		int rxY = HEIGHT - int(HEIGHT * (mSpeedHistoryRx[i]/mMaxSpeed));
		int txY = HEIGHT - int(HEIGHT * (mSpeedHistoryTx[i]/mMaxSpeed));
		paint.setPen(mOptions.mChartDldColor);
		paint.drawLine(lastX, lastRxY, x, rxY);
		paint.setPen(mOptions.mChartUplColor);
		paint.drawLine(lastX, lastTxY, x, txY);
		//qDebug("%d => %d", i, int(mSpeedHistoryRx[i]));
		lastX = x;
		lastRxY = rxY;
		lastTxY = txY;

		count++;
		x = width()-int(count+1);
	}
}

void KNetStatsView::mousePressEvent(QMouseEvent* ev) {
	if (ev->button() == Qt::RightButton )
		mContextMenu->exec(QCursor::pos());
	else if (ev->button() == Qt::LeftButton)
		if (mStatistics->isShown())
			mStatistics->accept();
		else
			mStatistics->show();
}

bool KNetStatsView::openFdSocket() {
	if (mFdSock > 0)
		return true;
	if ((mFdSock = socket(AF_INET, SOCK_DGRAM, 0)) > 0)
		return true;
	return false;
}

QString KNetStatsView::getIp() {
	if (mFdSock == -1 && !openFdSocket())
		return "";

	ioctl(mFdSock, SIOCGIFADDR, &mDevInfo);
	sockaddr_in sin = ((sockaddr_in&)mDevInfo.ifr_addr);
	return inet_ntoa(sin.sin_addr);
}

QString KNetStatsView::getNetmask() {
	if (mFdSock == -1 && !openFdSocket())
		return "";
	ioctl(mFdSock, SIOCGIFNETMASK, &mDevInfo);
	sockaddr_in mask = ((sockaddr_in&)mDevInfo.ifr_netmask);
	return inet_ntoa(mask.sin_addr);
}

void KNetStatsView::readOptions( const QString& name, KNetStatsView::Options* opts, bool defaultVisibility ) {
	KConfig* cfg = kapp->config();
	KConfigGroupSaver groupSaver(cfg, name);

	// general
	opts->mUpdateInterval = cfg->readNumEntry("UpdateInterval", 300);
	opts->mViewMode = (Mode)cfg->readNumEntry("ViewMode", 0);
	opts->mMonitoring = cfg->readBoolEntry("Monitoring", defaultVisibility);
	// txt view
	opts->mTxtFont = cfg->readFontEntry("TxtFont");
	opts->mTxtUplColor = cfg->readColorEntry("TxtUplColor", &Qt::red);
	opts->mTxtDldColor = cfg->readColorEntry("TxtDldColor", &Qt::green);
	// IconView
	int defaultTheme = 0;
	if (name.startsWith("wlan"))
		defaultTheme = 3; // three... is a magic number... a magic number....
	opts->mTheme = cfg->readNumEntry("Theme", defaultTheme);
	// Graphic
	opts->mChartUplColor = cfg->readColorEntry("ChartUplColor", &Qt::red);
	opts->mChartDldColor = cfg->readColorEntry("ChartDldColor", &Qt::blue);
	opts->mChartBgColor = cfg->readColorEntry("ChartBgColor", &Qt::white);
	opts->mChartTransparentBackground = cfg->readBoolEntry("ChartUseTransparentBackground", true);
}


#include "knetstatsview.moc"
