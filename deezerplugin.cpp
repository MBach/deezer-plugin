#include "deezerplugin.h"
#include "settings.h"
#include "webview.h"
#include "cookiejar.h"

#include <QDesktopServices>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QWebView>

#include <QtDebug>

DeezerPlugin::DeezerPlugin()
	: QObject(), _networkAccessManager(new QNetworkAccessManager(this))

{
	_networkAccessManager->setCookieJar(new CookieJar(this));
}

DeezerPlugin::~DeezerPlugin()
{
	foreach (QWebView *view, _pages) {
		delete view;
		view = NULL;
	}
	_pages.clear();
}

QWidget* DeezerPlugin::configPage()
{
	QWidget *widget = new QWidget();
	_config.setupUi(widget);
	
	Settings *settings = Settings::getInstance();
	bool b = settings->value("DeezerPlugin/save").toBool();
	_config.saveCredentialsCheckBox->setChecked(b);
	if (b) {
		QByteArray lba = settings->value("DeezerPlugin/l").toByteArray();
		QByteArray pba = settings->value("DeezerPlugin/p").toByteArray();
		_config.loginLineEdit->setText(QByteArray::fromBase64(lba));
		_config.passwordLineEdit->setText(QByteArray::fromBase64(pba));
	}
	
	connect(_config.saveCredentialsCheckBox, &QCheckBox::toggled, this, &DeezerPlugin::saveCredentials);
	connect(_config.openConnectPopup, &QPushButton::clicked, this, &DeezerPlugin::login);
	_config.logo->installEventFilter(this);
	return widget;
}

bool DeezerPlugin::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
		QDesktopServices::openUrl(QUrl("http://www.deezer.com"));
		return true;
    } else {
        return QObject::eventFilter(obj, event);
    }
}

void DeezerPlugin::setMediaPlayer(QWeakPointer<MediaPlayer> mediaPlayer)
{
	_mediaPlayer = mediaPlayer;
}

void DeezerPlugin::saveCredentials(bool enabled)
{
	Settings *settings = Settings::getInstance();
	if (enabled) {
		QByteArray lba(_config.loginLineEdit->text().toStdString().data());
		QByteArray pba(_config.passwordLineEdit->text().toStdString().data());
		settings->setValue("DeezerPlugin/l", lba.toBase64());
		settings->setValue("DeezerPlugin/p", pba.toBase64());
		settings->setValue("DeezerPlugin/save", enabled);
	} else {
		settings->remove("DeezerPlugin/l");
		settings->remove("DeezerPlugin/p");
		settings->remove("DeezerPlugin/save");
	}
}

void DeezerPlugin::login()
{
	qDebug() << Q_FUNC_INFO;
	WebView *webView = new WebView;
	_pages.append(webView);
	webView->page()->setNetworkAccessManager(_networkAccessManager);
	
	// HTML files from qrc:// does not work
	QNetworkRequest *init = new QNetworkRequest(QUrl("http://mbach.github.io/Miam-Player/deezer-light/index.html"));
	webView->load(*init);
	webView->show();
}
