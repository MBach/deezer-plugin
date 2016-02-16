/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef WEBVIEW_H
#define WEBVIEW_H

#include <QIcon>
#include <QWebEngineView>

QT_BEGIN_NAMESPACE
class QAuthenticator;
class QMouseEvent;
class QNetworkProxy;
class QNetworkReply;
class QSslError;
QT_END_NAMESPACE

/*class WebPage : public QWebEnginePage
{
	Q_OBJECT

signals:
	void loadingUrl(const QUrl &url);

public:
	WebPage(QWebEngineProfile *profile, QObject *parent = 0);

protected:
	virtual bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame) override;
	virtual bool certificateError(const QWebEngineCertificateError &error) override;
	virtual QWebEnginePage *createWindow(WebWindowType);

private:
	friend class WebView;
	QUrl m_loadingUrl;
	bool _hasToken;

signals:
	void tokenFound(const QString &token);
};*/

class DeezerPlugin;

class WebView : public QWebEngineView
{
	Q_OBJECT
private:
//	QString _token;
	DeezerPlugin *_deezer;

public:
	WebView(DeezerPlugin *deezer, QWidget *parent = 0);

	//inline WebPage *webPage() const { return m_page; }
	//void setPage(WebPage *page);

	void loadUrl(const QUrl &url);

	//QString token() const;
	//void setToken(const QString &token);

signals:
	void aboutToSyncWithToken(const QString &token);

private slots:
	void loadFinished(bool success);

//private:
//	WebPage *m_page;
};

#endif
