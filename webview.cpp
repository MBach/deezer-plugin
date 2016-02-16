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

#include "webview.h"

#include <QMessageBox>
#include <QWebEngineProfile>

#include <QtCore/QDebug>
#include <QtCore/QBuffer>

/*WebPage::WebPage(QWebEngineProfile *profile, QObject *parent)
	: QWebEnginePage(profile, parent)
	, _hasToken(false)
{}

bool WebPage::acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame)
{
	Q_UNUSED(type);
	if (isMainFrame) {
		m_loadingUrl = url;
		emit loadingUrl(m_loadingUrl);
	}
	qDebug() << Q_FUNC_INFO << url.toString();
	if (url.toString().contains("access_token=")) {
		qDebug() << Q_FUNC_INFO << "access_token found";

		QString r = url.toString();
		int i = r.indexOf("access_token=");
		int j = r.mid(i + 13).indexOf('&');
		QString t = r.mid(i + 13, j);
		if (_hasToken == false) {
			_hasToken = true;
			WebView *view = qobject_cast<WebView*>(this->view());
			if (view) {
				qDebug() << Q_FUNC_INFO << "token found" << t;
				view->setToken(t);
			} else {
				qDebug() << Q_FUNC_INFO << "cannot cast view";
			}
		}
	}
	return true;
}

bool WebPage::certificateError(const QWebEngineCertificateError &error)
{
	qDebug() << Q_FUNC_INFO;
	if (error.isOverridable()) {
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.setText(error.errorDescription());
		msgBox.setInformativeText(tr("If you wish so, you may continue with an unverified certificate. "
									 "Accepting an unverified certificate means "
									 "you may not be connected with the host you tried to connect to.\n"
									 "Do you wish to override the security check and continue?"));
		msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		msgBox.setDefaultButton(QMessageBox::No);
		return msgBox.exec() == QMessageBox::Yes;
	}
	QMessageBox::critical(view(), tr("Certificate Error"), error.errorDescription(), QMessageBox::Ok, QMessageBox::NoButton);
	return false;
}

QWebEnginePage * WebPage::createWindow(WebWindowType)
{
	qDebug() << Q_FUNC_INFO;
	return new WebPage(this->profile(), this->parent());
}*/

#include "deezerplugin.h"

WebView::WebView(DeezerPlugin *deezer, QWidget* parent)
	: QWebEngineView(parent), _deezer(deezer)
	//, m_page(new WebPage(QWebEngineProfile::defaultProfile(), this->parent()))
{
	connect(this, &QWebEngineView::loadFinished, this, &WebView::loadFinished);
}

/*void WebView::setPage(WebPage *_page)
{
	qDebug() << Q_FUNC_INFO << _page;
	if (m_page) {
		m_page->deleteLater();
	}
	m_page = _page;
	QWebEngineView::setPage(_page);
	connect(m_page, &WebPage::loadingUrl, this, &WebView::urlChanged);
}*/

void WebView::loadFinished(bool success)
{
	qDebug() << Q_FUNC_INFO << success;
	if (success) {
		qDebug() << url();
		if (url().toString().contains("access_token=")) {
			qDebug() << Q_FUNC_INFO << "access_token found";
			QString r = url().toString();
			QString access = "access_token=";
			int i = r.indexOf(access);
			int j = r.mid(i + access.count()).indexOf('&');
			QString t = r.mid(i + access.count(), j);
			qDebug() << Q_FUNC_INFO << "token found" << t;
			_deezer->sync(t);
			this->hide();
		}
	}

}

void WebView::loadUrl(const QUrl &url)
{
	qDebug() << Q_FUNC_INFO << url.toString();
	load(url);
}

/*QString WebView::token() const
{
	return _token;
}

void WebView::setToken(const QString &token)
{
	_token = token;
	emit aboutToSyncWithToken(token);
}*/
