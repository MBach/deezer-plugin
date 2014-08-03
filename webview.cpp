#include "webview.h"

#include <QtDebug>

WebView::WebView(QWidget *parent) :
	QWebView(parent)
{
	settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
	settings()->setAttribute(QWebSettings::PluginsEnabled, true);
	settings()->setAttribute(QWebSettings::JavascriptEnabled, true);
	settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows, true);
	settings()->setThirdPartyCookiePolicy(QWebSettings::AlwaysAllowThirdPartyCookies);
}

#include <QWebElement>
#include <QWebFrame>

QWebView* WebView::createWindow(QWebPage::WebWindowType)
{
	WebView *w = new WebView;
	// Autoclose the popup
	connect(w->page(), &QWebPage::windowCloseRequested, this, [=]() {
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
		QWebElement we = w->page()->currentFrame()->documentElement();
		QWebElement login = we.findFirst("#login_mail");
		login.setAttribute("value", "TEST_LOGIN@domain.com");
		QWebElement password = we.findFirst("#login_password");
		password.setAttribute("value", "123456_what_else");
		qDebug() << login.toPlainText();
	});
	return w;
}
