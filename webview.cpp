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

#include "cookiejar.h"
#include "networkaccessmanager.h"
#include "webview.h"
#include "settings.h"

#include <QtGui/QClipboard>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtGui/QMouseEvent>

#include <QWebHitTestResult>

#include <QtCore/QDebug>
#include <QtCore/QBuffer>

#include <QNetworkReply>
#include <QWebElement>
#include <QWebSecurityOrigin>

bool WebPage::_hasToken = false;

WebPage::WebPage(QObject *parent)
	: QWebPage(parent)
	, m_keyboardModifiers(Qt::NoModifier)
	, m_pressedButtons(Qt::NoButton)
	, m_openInNewTab(false)
{
	setNetworkAccessManager(NetworkAccessManager::getInstance());
	connect(this, SIGNAL(unsupportedContent(QNetworkReply*)), this, SLOT(handleUnsupportedContent(QNetworkReply*)));
}

bool WebPage::acceptNavigationRequest(QWebFrame *, const QNetworkRequest &request, NavigationType)
{
	qDebug() << "acceptNavigationRequest" << request.url();
	if (request.url().toString().contains("access_token=")) {
		QString r = request.url().toString();
		int i = r.indexOf("access_token=");
		int j = r.mid(i + 13).indexOf('&');
		QString t = r.mid(i + 13, j);
		if (!_hasToken) {
			qDebug() << "token:" << t;
			emit tokenFound(t);
			_hasToken = true;
		}
	}
	return true;
}

QWebPage *WebPage::createWindow(QWebPage::WebWindowType)
{
	return new WebPage();
}

void WebPage::handleUnsupportedContent(QNetworkReply *reply)
{
	QString errorString = reply->errorString();

	if (m_loadingUrl != reply->url()) {
		// sub resource of this page
		qWarning() << "Resource" << reply->url().toEncoded() << "has unknown Content-Type, will be ignored.";
		reply->deleteLater();
		return;
	}

	if (reply->error() == QNetworkReply::NoError && !reply->header(QNetworkRequest::ContentTypeHeader).isValid()) {
		errorString = "Unknown Content-Type";
	}

	QFile file(QLatin1String(":/notfound.html"));
	bool isOpened = file.open(QIODevice::ReadOnly);
	Q_ASSERT(isOpened);
	Q_UNUSED(isOpened)

	QString title = tr("Error loading page: %1").arg(reply->url().toString());
	QString html = QString(QLatin1String(file.readAll()))
						.arg(title)
						.arg(errorString)
						.arg(reply->url().toString());

	QBuffer imageBuffer;
	imageBuffer.open(QBuffer::ReadWrite);
	QIcon icon = view()->style()->standardIcon(QStyle::SP_MessageBoxWarning, 0, view());
	QPixmap pixmap = icon.pixmap(QSize(32,32));
	if (pixmap.save(&imageBuffer, "PNG")) {
		html.replace(QLatin1String("IMAGE_BINARY_DATA_HERE"),
					 QString(QLatin1String(imageBuffer.buffer().toBase64())));
	}

	QList<QWebFrame*> frames;
	frames.append(mainFrame());
	while (!frames.isEmpty()) {
		QWebFrame *frame = frames.takeFirst();
		if (frame->url() == reply->url()) {
			frame->setHtml(html, reply->url());
			return;
		}
		QList<QWebFrame *> children = frame->childFrames();
		foreach(QWebFrame *frame, children)
			frames.append(frame);
	}
	if (m_loadingUrl == reply->url()) {
		mainFrame()->setHtml(html, reply->url());
	}
}

QString WebView::token = QString();

WebView::WebView(QWidget* parent)
	: QWebView(parent)
	, m_progress(0)
	, m_page(new WebPage(this))
{
	m_page->setNetworkAccessManager(NetworkAccessManager::getInstance());
	m_page->mainFrame()->securityOrigin().addAccessWhitelistEntry("https://", "https://www.deezer.com", QWebSecurityOrigin::AllowSubdomains);
	m_page->mainFrame()->securityOrigin().addAccessWhitelistEntry("http://", "http://www.deezer.com", QWebSecurityOrigin::AllowSubdomains);
	m_page->mainFrame()->securityOrigin().addAccessWhitelistEntry("https://", "www.deezer.com", QWebSecurityOrigin::AllowSubdomains);
	m_page->mainFrame()->securityOrigin().addAccessWhitelistEntry("http://", "www.deezer.com", QWebSecurityOrigin::AllowSubdomains);
	setPage(m_page);
	connect(m_page, &WebPage::tokenFound, this, &WebView::tokenFound);
	connect(page(), SIGNAL(statusBarMessage(QString)), SLOT(setStatusBarText(QString)));
	connect(this, SIGNAL(loadProgress(int)), this, SLOT(setProgress(int)));
	connect(this, SIGNAL(loadFinished(bool)), this, SLOT(loadFinished()));
	page()->setForwardUnsupportedContent(true);
}

void WebView::setProgress(int progress)
{
	m_progress = progress;
}

void WebView::loadFinished()
{
	if (100 != m_progress) {
		qWarning() << "Received finished signal while progress is still:" << progress()
				   << "Url:" << url();
	}
	m_progress = 0;
}

void WebView::loadUrl(const QUrl &url)
{
	m_initialUrl = url;
	load(url);
}

QString WebView::lastStatusBarText() const
{
	return m_statusBarText;
}

QUrl WebView::url() const
{
	QUrl url = QWebView::url();
	if (!url.isEmpty()) {
		qDebug() << "url not empty";
		return url;
	}
	qDebug() << "initial url";
	return m_initialUrl;
}

void WebView::setStatusBarText(const QString &string)
{
	m_statusBarText = string;
}

QWebView* WebView::createWindow(QWebPage::WebWindowType)
{
	WebView *w = new WebView;
	w->page()->setNetworkAccessManager(NetworkAccessManager::getInstance());

	// Autoclose the popup
	connect(w->page(), &QWebPage::windowCloseRequested, this, [=]() {
		qDebug() << "autoclose";
		w->close();
	});
	if (w->page() && w->page()->currentFrame()) {
		qDebug() << "here";
	}
	connect(w, &QWebView::loadFinished, this, [=](bool ok) {
		qDebug() << "ok" << ok;
		if (!ok) {
			return;
		}
		Settings *s = Settings::getInstance();
		QByteArray lba = s->value("DeezerPlugin/l").toByteArray();
		QByteArray pba = s->value("DeezerPlugin/p").toByteArray();

		QWebElement we = w->page()->currentFrame()->documentElement();
		QWebElement login = we.findFirst("#login_mail");
		login.setAttribute("value", QByteArray::fromBase64(lba));
		QWebElement password = we.findFirst("#login_password");
		password.setAttribute("value", QByteArray::fromBase64(pba));
		qDebug() << login.toPlainText();
	});
	return w;
}
