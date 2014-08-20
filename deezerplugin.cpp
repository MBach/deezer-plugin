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

#include <QtDebug>

DeezerPlugin::DeezerPlugin()
	: QObject(), _artists(NULL), _albums(NULL), _tracks(NULL), _searchDialog(NULL),
	  _checkBox(NULL), _webPlayer(new DeezerWebPlayer(this))
{
	NetworkAccessManager *nam = NetworkAccessManager::getInstance();
	nam->setCookieJar(new CookieJar);

	// Dispatch replies: search for something, get artist info, get tracks from album, get track info
	connect(nam, &QNetworkAccessManager::finished, this, [=](QNetworkReply *reply) {
		QNetworkRequest request = reply->request();
		QString r = request.url().toDisplayString();
		// qDebug() << request.url();
		if (r.startsWith("http://api.deezer.com/album")) {
			QByteArray ba = reply->readAll();
			QXmlStreamReader xml(ba);
			this->extractAlbum(xml);
		} else if (r.startsWith("http://api.deezer.com/user/me/artists")) {
			QByteArray ba = reply->readAll();
			qDebug() << ba;
			//QXmlStreamReader xml(ba);
			/// JSON?
			//this->extractSynchronizedArtists(xml);
		} else {
			QByteArray ba = reply->readAll();
			QXmlStreamReader xml(ba);
			this->searchRequestFinished(xml);
		}
	});

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
	connect(this, &DeezerPlugin::aboutToProcessRemoteTracks, w, &AbstractSearchDialog::aboutToProcessRemoteTracks);

	_searchDialog = w;
	_searchDialog->addSource(_checkBox);
	_artists = w->artists();
	_albums = w->albums();
	_tracks = w->tracks();

	connect(_artists, &QListView::doubleClicked, this, &DeezerPlugin::artistWasDoubleClicked);
	connect(_albums, &QListView::doubleClicked, this, &DeezerPlugin::albumWasDoubleClicked);
	connect(_tracks, &QListView::doubleClicked, this, &DeezerPlugin::trackWasDoubleClicked);
}

void DeezerPlugin::sync() const
{
	qDebug() << Q_FUNC_INFO;
	qDebug() << "get remote library from Deezer";
	//while ()
	//NetworkAccessManager::getInstance()->get(QNetworkRequest(QUrl("http://api.deezer.com/user/me/artists")));
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

void DeezerPlugin::extractAlbum(QXmlStreamReader &xml)
{
	std::list<RemoteTrack> tracks;
	bool artistFound = false;
	bool albumFound = false;
	QString artist;
	QString album;
	QString year;
	int trackNumber = 1;
	static QIcon icon(":/icon");
	while (!xml.atEnd() && !xml.hasError()) {

		QXmlStreamReader::TokenType token = xml.readNext();

		// Parse start elements
		if (token == QXmlStreamReader::StartElement) {
			RemoteTrack track;
			if (xml.name() == "title") {
				album = xml.readElementText();
				albumFound = true;
			}
			if (xml.name() == "artist") {
				while (xml.name() != "name" && !xml.hasError()) {
					xml.readNext();
				}
				artist = xml.readElementText();
				artistFound = true;
			}
			if (xml.name() == "release_date") {
				year = xml.readElementText();
			}

			if (artistFound && albumFound && xml.name() == "track") {
				while (xml.name() != "id" && !xml.hasError()) {
					xml.readNext();
				}
				track.setId(xml.readElementText());
				while (xml.name() != "title" && !xml.hasError()) {
					xml.readNext();
				}
				track.setTitle(xml.readElementText());
				while (xml.name() != "link" && !xml.hasError()) {
					xml.readNext();
				}
				track.setUrl(xml.readElementText());
				while (xml.name() != "duration" && !xml.hasError()) {
					xml.readNext();
				}
				track.setLength(xml.readElementText());
				while (xml.name() != "rank" && !xml.hasError()) {
					xml.readNext();
				}
				int r = xml.readElementText().toInt();
				r = round((double)r / 166666);
				track.setRating(r);

				track.setArtist(artist);
				track.setAlbum(album);
				track.setIcon(icon);
				/// FIXME. Example, Broken by Nine Inch Nails -> # are (1, ..., 6, 98, 99)
				track.setTrackNumber(QString::number(trackNumber++));
				track.setYear(year.left(4));
				tracks.push_back(std::move(track));
			}
		}
	}
	emit aboutToProcessRemoteTracks(tracks);
}

void DeezerPlugin::extractSynchronizedArtists(QXmlStreamReader &xml)
{
	qDebug() << Q_FUNC_INFO;
	//qDebug() << xml.t;
}

void DeezerPlugin::searchRequestFinished(QXmlStreamReader &xml)
{
	QList<QStandardItem*> results;

	while(!xml.atEnd() && !xml.hasError()) {

		QXmlStreamReader::TokenType token = xml.readNext();

		// Parse start elements
		if (token == QXmlStreamReader::StartElement) {
			if (xml.name() == "album") {
				QString element;
				QString id;
				while (xml.name() != "id" && !xml.hasError()) {
					xml.readNext();
				}
				id = xml.readElementText();
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
					QStandardItem *item = new QStandardItem(QIcon(":/icon"), element);
					item->setData(_checkBox->text(), AbstractSearchDialog::DT_Origin);
					item->setData(id, AbstractSearchDialog::DT_Identifier);
					results.append(item);
				}
			}
		}
	}
	emit searchComplete(AbstractSearchDialog::Album, results);
}

void DeezerPlugin::artistWasDoubleClicked(const QModelIndex &index)
{
	QStandardItemModel *m = qobject_cast<QStandardItemModel*>(_artists->model());
	if (m->itemFromIndex(index)->data(AbstractSearchDialog::DT_Origin) == _checkBox->text()) {
		qDebug() << Q_FUNC_INFO << "not implemented";
		qDebug() << m->itemFromIndex(index)->text();
	}
}

void DeezerPlugin::albumWasDoubleClicked(const QModelIndex &index)
{
	QStandardItemModel *m = qobject_cast<QStandardItemModel*>(_albums->model());
	QStandardItem *item = m->itemFromIndex(index);
	// Filter items built by this plugin only
	if (item->data(AbstractSearchDialog::DT_Origin) == _checkBox->text()) {

		// Get album info: at least all the tracks
		QString idAlbum = item->data(AbstractSearchDialog::DT_Identifier).toString();

		NetworkAccessManager::getInstance()->get(QNetworkRequest(QUrl("http://api.deezer.com/album/" + idAlbum + "?output=xml")));
	}
}

void DeezerPlugin::trackWasDoubleClicked(const QModelIndex &index)
{
	QStandardItemModel *m = qobject_cast<QStandardItemModel*>(_tracks->model());
	if (m->itemFromIndex(index)->data(AbstractSearchDialog::DT_Origin) == _checkBox->text()) {
		qDebug() << Q_FUNC_INFO << "not implemented";
		qDebug() << m->itemFromIndex(index)->text();
	}
}

void DeezerPlugin::search(const QString &expr)
{
	if (!_checkBox->isChecked()) {
		return;
	}

	// Do not spam remote location with single character request
	if (expr.length() <= 1) {
		return;
	}

	/// XXX: escape this
	NetworkAccessManager::getInstance()->get(QNetworkRequest(QUrl("http://api.deezer.com/search/album?output=xml&limit=5&q=" + expr)));
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

#include <QWebFrame>

void DeezerPlugin::login()
{
	qDebug() << Q_FUNC_INFO;
	WebView *webView = new WebView;
	webView->loadUrl(QUrl("https://connect.deezer.com/oauth/auth.php?app_id=141475&format=popup&redirect_uri=http://mbach.github.io/Miam-Player/deezer-light/channel.html&response_type=token&scope=manage_library,basic_access"));
	webView->show();

	_pages.append(webView);
}
