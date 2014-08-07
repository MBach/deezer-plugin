#include "deezerplugin.h"
#include "settings.h"
#include "webview.h"
#include "cookiejar.h"
#include "networkaccessmanager.h"

#include <QDesktopServices>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QWebView>
#include <QXmlStreamReader>

#include <QtDebug>

DeezerPlugin::DeezerPlugin()
	: QObject()
{
	NetworkAccessManager::getInstance()->setCookieJar(new CookieJar);
	QWebSettings *s = QWebSettings::globalSettings();
	/// XXX
	s->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
	s->setAttribute(QWebSettings::PluginsEnabled, true);
	s->setAttribute(QWebSettings::JavascriptEnabled, true);
	s->setAttribute(QWebSettings::JavascriptCanOpenWindows, true);
	s->setAttribute(QWebSettings::JavascriptCanCloseWindows, true);
	s->setAttribute(QWebSettings::DnsPrefetchEnabled, true);
	s->setAttribute(QWebSettings::LocalStorageEnabled, true);
	s->setAttribute(QWebSettings::LocalContentCanAccessRemoteUrls, true);
	s->setAttribute(QWebSettings::LocalContentCanAccessFileUrls, true);
	s->setThirdPartyCookiePolicy(QWebSettings::AlwaysAllowThirdPartyCookies);
}

DeezerPlugin::~DeezerPlugin()
{
	foreach (QWebView *view, _pages) {
		delete view;
		view = NULL;
	}
	_pages.clear();
}

QStringList DeezerPlugin::classesToExtend()
{
	QStringList l;
	l << "QListWidget" << "QWidget";
	return l;
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

void DeezerPlugin::init()
{
	qDebug() << Q_FUNC_INFO;
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

void DeezerPlugin::replyFinished(QNetworkReply *reply)
{
	QByteArray ba = reply->readAll();
	qDebug() << Q_FUNC_INFO << ba;
	/*QXmlStreamReader xml(ba);
	while(!xml.atEnd() && !xml.hasError()) {

		QXmlStreamReader::TokenType token = xml.readNext();

		// Parse start elements
		if (token == QXmlStreamReader::StartElement) {
			if (xml.name() == "release-group") {
				if (xml.attributes().hasAttribute("id")) {
					QStringRef sr = xml.attributes().value("id");
					if (xml.readNextStartElement() && xml.name() == "title") {
						map.insert(xml.readElementText().toLower(), sr.toString());
					}
				}
			}
		}
	}*/
}

void DeezerPlugin::search(const QString &expr)
{
	qDebug() << Q_FUNC_INFO;
	QNetworkRequest r;
	r.setUrl(QUrl("http://api.deezer.com/search/album?q=" + expr));
	NetworkAccessManager::getInstance()->get(r);
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

	// HTML files from qrc:// does not work
	webView->loadUrl(QUrl("http://mbach.github.io/Miam-Player/deezer-light/index.html"));
	webView->show();

	_pages.append(webView);
}
