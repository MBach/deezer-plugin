/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "networkaccessmanager.h"

//#include "browserapplication.h"
//#include "browsermainwindow.h"
//#include "ui_passworddialog.h"
//#include "ui_proxy.h"

#include <QtCore/QSettings>

#include <QtGui/QDesktopServices>
#include <QtWidgets/QDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QStyle>
#include <QtGui/QTextDocument>

#include <QtNetwork/QAuthenticator>
#include <QtNetwork/QNetworkDiskCache>
#include <QtNetwork/QNetworkProxy>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QSslError>

#include <QThread>

NetworkAccessManager* NetworkAccessManager::networkAccessManager = NULL;

int NetworkAccessManager::pendingRequests = 0;

NetworkAccessManager::NetworkAccessManager(QObject *parent)
	: QNetworkAccessManager(parent),
	requestFinishedCount(0), requestFinishedFromCacheCount(0), requestFinishedPipelinedCount(0),
	requestFinishedSecureCount(0), requestFinishedDownloadBufferCount(0)
{
	//connect(this, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)), SLOT(authenticationRequired(QNetworkReply*,QAuthenticator*)));
	//connect(this, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)), SLOT(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)));
	//connect(this, &QNetworkAccessManager::finished, this, &NetworkAccessManager::requestFinished);

	//connect(this, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), SLOT(sslErrors(QNetworkReply*,QList<QSslError>)));
	connect(this, &QNetworkAccessManager::sslErrors, this, &NetworkAccessManager::ignoreSslErrors);

	loadSettings();

	QNetworkDiskCache *diskCache = new QNetworkDiskCache(this);
	QString location = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
	diskCache->setCacheDirectory(location);
	setCache(diskCache);
}

NetworkAccessManager* NetworkAccessManager::getInstance()
{
	if (networkAccessManager == NULL) {
		networkAccessManager = new NetworkAccessManager;
	}
	return networkAccessManager;
}

QNetworkReply* NetworkAccessManager::createRequest(Operation op, const QNetworkRequest & req, QIODevice * outgoingData)
{
	/*while (pendingRequests > 4) {
		qDebug() << "waiting for 1 second";
		this->thread()->sleep(1);
	}*/
	//if (pendingRequests > 4) {
	//	qDebug() << "waiting for 1 second";
	//	this->thread()->sleep(1);
	//}
	QNetworkRequest request = req; // copy so we can modify
	qDebug() << "create request" << pendingRequests;

	// this is a temporary hack until we properly use the pipelining flags from QtWebkit
	// pipeline everything! :)
	request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
	QNetworkReply *r = QNetworkAccessManager::createRequest(op, request, outgoingData);
	//pendingRequests++;
	return r;
}

QNetworkReply * NetworkAccessManager::get(const QNetworkRequest & request)
{
	static int getCount = 0;
	qDebug() << Q_FUNC_INFO << ++getCount;
	return QNetworkAccessManager::get(request);
}

QNetworkReply * NetworkAccessManager::post(const QNetworkRequest & request, QIODevice * data)
{
	qDebug() << Q_FUNC_INFO;
	return QNetworkAccessManager::post(request, data);
}

QNetworkReply * NetworkAccessManager::post(const QNetworkRequest & request, const QByteArray & data)
{
	qDebug() << Q_FUNC_INFO;
	return QNetworkAccessManager::post(request, data);
}

QNetworkReply * NetworkAccessManager::post(const QNetworkRequest & request, QHttpMultiPart * multiPart)
{
	qDebug() << Q_FUNC_INFO;
	return QNetworkAccessManager::post(request, multiPart);
}

QNetworkReply * NetworkAccessManager::put(const QNetworkRequest & request, QIODevice * data)
{
	qDebug() << Q_FUNC_INFO;
	return QNetworkAccessManager::put(request, data);
}

QNetworkReply * NetworkAccessManager::put(const QNetworkRequest & request, QHttpMultiPart * multiPart)
{
	qDebug() << Q_FUNC_INFO;
	return QNetworkAccessManager::put(request, multiPart);
}

QNetworkReply * NetworkAccessManager::put(const QNetworkRequest & request, const QByteArray & data)
{
	qDebug() << Q_FUNC_INFO;
	return QNetworkAccessManager::put(request, data);
}

void NetworkAccessManager::loadSettings()
{
	QSettings settings;
	settings.beginGroup(QLatin1String("proxy"));
	QNetworkProxy proxy;
	if (settings.value(QLatin1String("enabled"), false).toBool()) {
		if (settings.value(QLatin1String("type"), 0).toInt() == 0)
			proxy = QNetworkProxy::Socks5Proxy;
		else
			proxy = QNetworkProxy::HttpProxy;
		proxy.setHostName(settings.value(QLatin1String("hostName")).toString());
		proxy.setPort(settings.value(QLatin1String("port"), 1080).toInt());
		proxy.setUser(settings.value(QLatin1String("userName")).toString());
		proxy.setPassword(settings.value(QLatin1String("password")).toString());
	}
	setProxy(proxy);
}

void NetworkAccessManager::ignoreSslErrors(QNetworkReply *reply, const QList<QSslError> &)
{
	// check if SSL certificate has been trusted already
	QString replyHost = reply->url().host() + QString(":%1").arg(reply->url().port());
	if (!sslTrustedHostList.contains(replyHost)) {
		//BrowserMainWindow *mainWindow = BrowserApplication::instance()->mainWindow();
		reply->ignoreSslErrors();
		sslTrustedHostList.append(replyHost);
	}
}
