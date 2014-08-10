#include "deezerplugin.h"
#include "settings.h"
#include "webview.h"
#include "cookiejar.h"
#include "networkaccessmanager.h"
#include "abstractsearchdialog.h"

#include <QDesktopServices>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QWebView>
#include <QXmlStreamReader>

#include <QtDebug>

DeezerPlugin::DeezerPlugin()
	: QObject(), _searchDialog(NULL), _checkBox(NULL)
{
	NetworkAccessManager *nam = NetworkAccessManager::getInstance();
	nam->setCookieJar(new CookieJar);
	connect(nam, &QNetworkAccessManager::finished, this, &DeezerPlugin::replyFinished);

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
	qDebug() << Q_FUNC_INFO;
	foreach (QWebView *view, _pages) {
		delete view;
		view = NULL;
	}
	_pages.clear();
	delete _checkBox;
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

void DeezerPlugin::setSearchDialog(AbstractSearchDialog *w)
{
	_checkBox = new QCheckBox;
	_checkBox->setChecked(true);
	_checkBox->setText("Deezer");
	connect(w, &AbstractSearchDialog::aboutToSearch, this, &DeezerPlugin::search);
	connect(this, &DeezerPlugin::searchComplete, w, &AbstractSearchDialog::processResults);

	_searchDialog = w;
	_searchDialog->addSource(_checkBox);
	_artists = w->artists();
	_albums = w->albums();
	_tracks = w->tracks();
}

void DeezerPlugin::dispatchResults(Request request, QListWidget *list)
{
	switch (request) {
	case Artist:
		_artists = list;
		break;
	case Album:
		_albums = list;
		break;
	case Track:
		_tracks = list;
		break;
	}
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
	QXmlStreamReader xml(ba);

	QList<QListWidgetItem*> results;

	while(!xml.atEnd() && !xml.hasError()) {

		QXmlStreamReader::TokenType token = xml.readNext();

		// Parse start elements
		if (token == QXmlStreamReader::StartElement) {
			if (xml.name() == "album") {
				QString element;
				while (xml.name() != "title" && !xml.hasError()) {
					xml.readNext();
				}
				element = xml.readElementText();
				while (xml.name() != "artist" && !xml.hasError()) {
					xml.readNext();
				}
				while (xml.name() != "name" && !xml.hasError()) {
					xml.readNext();
				}
				if (!element.isEmpty()) {
					element += " – " + xml.readElementText();
					qDebug() << "deezer-album" << element;
					QListWidgetItem *item = new QListWidgetItem(QIcon(":/icon"), element);
					results.append(item);
				}
			}
		}
	}
	emit searchComplete(SearchMediaPlayerPlugin::Album, results);
}

void DeezerPlugin::search(const QString &expr)
{
	if (!_checkBox->isChecked()) {
		qDebug() << "Deezer is currenlty disabled";
		return;
	}

	// Do not spam remote location with single character request
	if (expr.length() <= 1) {
		return;
	}

	QNetworkRequest r;
	/// XXX: escape this
	QString strRequest = "http://api.deezer.com/search/album?output=xml&limit=5&q=" + expr;

	qDebug() << Q_FUNC_INFO << "request" << strRequest;

	r.setUrl(QUrl(strRequest));
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
