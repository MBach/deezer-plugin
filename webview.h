#ifndef WEBVIEW_H
#define WEBVIEW_H

#include <QWebView>

class WebView : public QWebView
{
	Q_OBJECT
public:
	explicit WebView(QWidget *parent = 0);
	
protected:
	virtual QWebView* createWindow(QWebPage::WebWindowType);

};

#endif // WEBVIEW_H
