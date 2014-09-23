#include "deezerplugin.h"
#include "settings.h"
#include "webview.h"
#include "cookiejar.h"
#include "networkaccessmanager.h"
#include "abstractsearchdialog.h"

#include <QDesktopServices>
#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QWebView>

#include <QtDebug>

/** Default constructor. */
DeezerPlugin::DeezerPlugin()
	: QObject(), _artists(NULL), _albums(NULL), _tracks(NULL), _searchDialog(NULL),
	  _checkBox(NULL), _webPlayer(new DeezerWebPlayer(this))
{
	NetworkAccessManager *nam = NetworkAccessManager::getInstance();
	nam->setCookieJar(new CookieJar);

	// Dispatch replies: search for something, get artist info, get tracks from album, get track info, fetch, synchronize...
	connect(nam, &QNetworkAccessManager::finished, this, &DeezerPlugin::dispatchReply);

	connect(_webPlayer->webView(), &WebView::aboutToSyncWithToken, this, &DeezerPlugin::sync);
	/*connect(_webPlayer->webView(), &WebView::aboutToSyncWithToken, this, [=](const QString &token) {
		qDebug() << "about to sync";
		this->sync(token);
	});*/

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

	Settings *settings = Settings::getInstance();
	if (settings->value("DeezerPlugin/syncOptions").isNull()) {
		QStringList l = QStringList() << "album" << "ep" << "single";
		settings->setValue("DeezerPlugin/syncOptions", l);
	}
}

/** Default destructor. */
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

/** Load and return user interface to manipulate this plugin. */
QWidget* DeezerPlugin::configPage()
{
	Settings *settings = Settings::getInstance();
	QWidget *widget = new QWidget;
	_config.setupUi(widget);

	bool b = settings->value("DeezerPlugin/save").toBool();
	_config.saveCredentialsCheckBox->setChecked(b);
	if (b) {
		QByteArray lba = settings->value("DeezerPlugin/l").toByteArray();
		QByteArray pba = settings->value("DeezerPlugin/p").toByteArray();
		_config.loginLineEdit->setText(QByteArray::fromBase64(lba));
		_config.passwordLineEdit->setText(QByteArray::fromBase64(pba));
	}

	// Load settings
	_config.syncArtistsEPCheckbox->setChecked(settings->value("DeezerPlugin/syncArtistsEPCheckbox", true).toBool());
	_config.syncArtistsSinglesCheckbox->setChecked(settings->value("DeezerPlugin/syncArtistsSinglesCheckbox", false).toBool());

	// Connect elements from UI
	connect(_config.saveCredentialsCheckBox, &QCheckBox::toggled, this, &DeezerPlugin::saveCredentials);
	connect(_config.openConnectPopup, &QPushButton::clicked, this, &DeezerPlugin::login);
	connect(_config.syncArtistsEPCheckbox, &QCheckBox::toggled, this, [=](bool b) {
		QStringList l = settings->value("DeezerPlugin/syncOptions").toStringList();
		b ? l.append("ep") : l.removeAll("ep");
		settings->setValue("DeezerPlugin/syncOptions", l);
		settings->setValue("DeezerPlugin/syncArtistsEPCheckbox", b);
	});
	connect(_config.syncArtistsSinglesCheckbox, &QCheckBox::toggled, this, [=](bool b) {
		QStringList l = settings->value("DeezerPlugin/syncOptions").toStringList();
		b ? l.append("single") : l.removeAll("single");
		settings->setValue("DeezerPlugin/syncOptions", l);
		settings->setValue("DeezerPlugin/syncArtistsSinglesCheckbox", b);
	});

	// Detect click mouse event to open Deezer in browser
	_config.logo->installEventFilter(this);
	return widget;
}

/** Redefined. */
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

/** Redefined. */
void DeezerPlugin::sync(const QString &token) const
{
	qDebug() << Q_FUNC_INFO << "token empty?" << token.isEmpty();
	if (!token.isEmpty()) {
		qDebug() << Q_FUNC_INFO << token;
		qDebug() << "Ready to synchronize Library with Deezer (fetching remote only)";
		// First, get checksum for Artists, Albums and Playlists
		/// TODO: playlists, albums (followers?)
		NetworkAccessManager::getInstance()->get(QNetworkRequest(QUrl("http://api.deezer.com/user/me/artists?output=xml&access_token=" + token)));
	}
}

/** Redefined to open Deezer's home page. */
bool DeezerPlugin::eventFilter(QObject *obj, QEvent *event)
{
	if (obj == _config.logo && event->type() == QEvent::MouseButtonPress) {
		QDesktopServices::openUrl(QUrl("http://www.deezer.com"));
		return true;
	} else if (event->type() == QEvent::Show) {
		qDebug() << "show event";
		return true;
	} else {
		return QObject::eventFilter(obj, event);
	}
}

QString DeezerPlugin::extract(QXmlStreamReader &xml, const QString &criterion)
{
	while (xml.name() != criterion && !xml.hasError()) {
		xml.readNext();
	}
	return xml.readElementText();
}

void DeezerPlugin::extractAlbum(QNetworkReply *reply, QXmlStreamReader &xml)
{
	qDebug() << Q_FUNC_INFO;

	bool artistFound = false;
	bool albumFound = false;
	bool albumIdFound = false;
	QString id;
	QString artist;
	QString album;
	QString year;
	RemoteTrack *track = new RemoteTrack;
	while (!xml.atEnd() && !xml.hasError()) {

		QXmlStreamReader::TokenType token = xml.readNext();

		// Parse start elements
		if (token == QXmlStreamReader::StartElement) {

			if (!albumIdFound) {
				id = this->extract(xml, "id");
				albumIdFound = true;
				qDebug() << "id" << id;
			}
			if (xml.name() == "title") {
				album = xml.readElementText();
				albumFound = true;
			}
			if (xml.name() == "artist") {
				artist = this->extract(xml, "name");
				artistFound = true;
			}
			if (xml.name() == "release_date") {
				year = xml.readElementText();
			}
			if (artistFound && albumFound) {
				track->setId(id);
				track->setAlbum(album);
				track->setArtist(artist);
				track->setYear(year);
				_cache.insert(id, track);

				QNetworkReply *nextReply = NetworkAccessManager::getInstance()->get(QNetworkRequest(QUrl("http://api.deezer.com/album/" + id + "/tracks/?output=xml")));
				if (_repliesWhichInteractWithUi.contains(reply)) {
					Reply forward = _repliesWhichInteractWithUi.take(reply);
					delete reply;
					_repliesWhichInteractWithUi.insert(nextReply, forward);
				}
				break;
			}
		}
	}
}

void DeezerPlugin::extractAlbumListFromArtist(QNetworkReply *reply, QXmlStreamReader &xml)
{
	qDebug() << Q_FUNC_INFO;
	QList<RemoteTrack*> albums;
	Settings *settings = Settings::getInstance();
	while(!xml.atEnd() && !xml.hasError()) {
		QXmlStreamReader::TokenType token = xml.readNext();
		if (token == QXmlStreamReader::StartElement) {
			if (xml.name() == "album") {
				RemoteTrack *album = new RemoteTrack;
				QString record_type;
				album->setId(this->extract(xml, "id"));
				album->setTitle(this->extract(xml, "title"));
				/// TODO
				this->extract(xml, "cover");
				//album.setIcon(xml.readElementText());
				record_type = this->extract(xml, "record_type");
				if (settings->value("DeezerPlugin/syncOptions").toStringList().contains(record_type)) {
					albums.append(album);
				}
			}
		}
	}
	if (_db.open()) {
		for (int i = 0; i < albums.size(); i++) {
			RemoteTrack *album = albums.at(i);
			QSqlQuery insert(_db);
			insert.prepare("INSERT INTO albums(id, name, cover) VALUES (?, ?, ?)");
			insert.addBindValue(album->id());
			insert.addBindValue(album->title());
			//https://api.deezer.com/album/<id>/image?size=big
			insert.addBindValue("todo");
			_cache.insert(album->id(), album);

			// Finally, extract track list
			/// XXX: too much requests?
			if (insert.exec()) {
				QNetworkReply *nextReply = NetworkAccessManager::getInstance()->get(QNetworkRequest(QUrl("http://api.deezer.com/album/" + album->id() + "/tracks/?output=xml")));
				if (_repliesWhichInteractWithUi.contains(reply)) {
					Reply forward =_repliesWhichInteractWithUi.take(reply);
					delete reply;
					qDebug() << "forwarding" << forward << nextReply;
					_repliesWhichInteractWithUi.insert(nextReply, forward);
				}
			}
		}
		_db.close();
	}
}

void DeezerPlugin::extractSynchronizedArtists(QXmlStreamReader &xml)
{
	qDebug() << Q_FUNC_INFO;
	Settings *settings = Settings::getInstance();
	bool needToSyncArtists = false;
	QVariant checkSum = settings->value("DeezerPlugin/artists/checksum");
	QString sum;
	QList<RemoteTrack> artists;
	while(!xml.atEnd() && !xml.hasError()) {
		QXmlStreamReader::TokenType token = xml.readNext();
		if (token == QXmlStreamReader::StartElement) {
			if (xml.name() == "artist") {
				RemoteTrack artist;
				artist.setId(this->extract(xml, "id"));
				artist.setArtist(this->extract(xml, "name"));
				artists.append(artist);
			}
			if (xml.name() == "checksum") {
				sum = xml.readElementText();
				// Out of sync
				if (checkSum.isNull() || checkSum.toString() != sum) {
					needToSyncArtists = true;
				}
			}
		}
	}
	if (needToSyncArtists || _db.isEmpty()) {
		if (_db.open()) {
			_db.exec("TRUNCATE TABLE artists");
			bool ok = true;
			for (int i = 0; i < artists.size(); i++) {
				RemoteTrack artist = artists.at(i);
				QSqlQuery insert(_db);
				insert.prepare("INSERT INTO artists(id, name) VALUES (?, ?)");
				insert.addBindValue(artist.id());
				insert.addBindValue(artist.artist());
				if (insert.exec()) {
					QNetworkRequest r(QUrl("http://api.deezer.com/artist/" + artist.id() + "/albums?output=xml"));
					QNetworkReply *reply = NetworkAccessManager::getInstance()->get(r);
					_repliesWhichInteractWithUi.insert(reply, RPL_UpdateCacheDatabase);
				} else {
					ok = false;
				}
			}
			_db.close();
			if (ok) {
				qDebug() << "sum" << sum;
				settings->setValue("DeezerPlugin/artists/checksum", sum);
			}
		}
	} else {
		qDebug() << "artists are synchronized!";
	}
}

void DeezerPlugin::extractTrackListFromAlbum(QNetworkReply *reply, const QString &albumID, QXmlStreamReader &xml)
{
	qDebug() << Q_FUNC_INFO << reply;
	static QIcon icon(":/icon");
	std::list<RemoteTrack> tracks;
	if (_cache.contains(albumID)) {
		RemoteTrack *templateTrack = _cache[albumID];
		qDebug() << albumID << templateTrack->artist() << templateTrack->album();
		while (!xml.atEnd() && !xml.hasError()) {
			QXmlStreamReader::TokenType token = xml.readNext();
			if (token == QXmlStreamReader::StartElement) {
				if (xml.name() == "track") {
					RemoteTrack track;
					track.setId(this->extract(xml, "id"));
					track.setTitle(this->extract(xml, "title"));
					track.setUrl(this->extract(xml, "link"));
					track.setLength(this->extract(xml, "duration"));
					track.setTrackNumber(this->extract(xml, "track_position"));
					track.setDisc(this->extract(xml, "disk_number"));
					int r = this->extract(xml, "rank").toInt();
					r = round((double)r / 166666);
					track.setRating(r);
					track.setArtist(templateTrack->artist());
					track.setAlbum(templateTrack->album());
					track.setIcon(icon);
					track.setYear(templateTrack->year().left(4));
					tracks.push_back(std::move(track));
				}
			}
		}
		if (_repliesWhichInteractWithUi.contains(reply)) {
			Reply value = _repliesWhichInteractWithUi.value(reply);
			switch (value) {
			case RPL_SendToCurrentPlaylist:
				emit aboutToProcessRemoteTracks(tracks);
				break;
			case RPL_UpdateCacheDatabase:
				this->updateCacheDatabase(tracks);
				break;
			}
			_repliesWhichInteractWithUi.remove(reply);
			delete reply;
		}
	}
}

void DeezerPlugin::artistWasDoubleClicked(const QModelIndex &index)
{
	QStandardItemModel *m = qobject_cast<QStandardItemModel*>(_artists->model());
	if (m->itemFromIndex(index)->data(AbstractSearchDialog::DT_Origin) == _checkBox->text()) {
		qDebug() << Q_FUNC_INFO << "not implemented";
		qDebug() << m->itemFromIndex(index)->text();
	}
}

/** Fetch detailed information about the Album and append it to playlist. */
void DeezerPlugin::albumWasDoubleClicked(const QModelIndex &index)
{
	QStandardItemModel *m = qobject_cast<QStandardItemModel*>(_albums->model());
	QStandardItem *item = m->itemFromIndex(index);
	// Filter items built by this plugin only
	if (item->data(AbstractSearchDialog::DT_Origin) == _checkBox->text()) {

		// Get album info: at least all the tracks
		QString idAlbum = item->data(AbstractSearchDialog::DT_Identifier).toString();
		QNetworkRequest r(QUrl("http://api.deezer.com/album/" + idAlbum + "?output=xml"));
		QNetworkReply *reply = NetworkAccessManager::getInstance()->get(r);
		_repliesWhichInteractWithUi.insert(reply, RPL_SendToCurrentPlaylist);
	}
}

/** Display everything! */
void DeezerPlugin::dispatchReply(QNetworkReply *reply)
{
	qDebug() << Q_FUNC_INFO << reply->request().url();
	qDebug() << Q_FUNC_INFO << reply->url();

	QNetworkRequest request = reply->request();
	QString r = request.url().toDisplayString();
	QXmlStreamReader xml(reply->readAll());

	/// WARNING: used by sync() and search() -> album/<id>/tracklist
	QRegularExpression reg("http://api.deezer.com/album/[0-9]+/tracks");
	if (r.startsWith("http://api.deezer.com/album/") && !r.contains(reg)) {
		qDebug() << r;
		this->extractAlbum(reply, xml);
	} else if (r.startsWith("http://api.deezer.com/album/") && r.contains(reg)) {
		int slash = r.indexOf("/", 28);
		this->extractTrackListFromAlbum(reply, r.mid(28, slash - 28), xml);
	} else if (r.startsWith("http://api.deezer.com/user/me/artists")) {
		this->extractSynchronizedArtists(xml);
	/// TODO !?
	//} else if (r.startsWith("http://api.deezer.com/user/me/albums")) {
	//} else if (r.startsWith("http://api.deezer.com/user/me/playlists")) {
	//} else if (r.startsWith("http://api.deezer.com/user/me/flow")) {
	//} else if (r.startsWith("http://api.deezer.com/user/me/following")) {
	//} else if (r.startsWith("http://api.deezer.com/user/me/followers")) {
	//} else if (r.startsWith("http://api.deezer.com/user/me/radio")) {
	//} else if (r.startsWith("http://api.deezer.com/user/me/tracks")) {
	} else if (r.startsWith("http://api.deezer.com/artist/")) {
		this->extractAlbumListFromArtist(reply, xml);
	} else if (r.startsWith("http://api.deezer.com/search/artist/")) {
		/// TODO
	} else if (r.startsWith("http://api.deezer.com/search/album/")) {
		this->searchRequestFinished(xml);
	} else if (r.startsWith("http://api.deezer.com/search/track/")) {
		/// TODO
	} else if (r.startsWith("http://api.deezer.com/")) {
		qDebug() << "unknown search request" << r;
	} else {
		if (reply->error() != QNetworkReply::NoError) {
			qDebug() << "Unknown reply. Code:" << reply->error() << "String:" << reply->errorString();
			qDebug() << "Reply url: " << reply->url() << ", requested url" << reply->request().url();
		}
	}
}

/** Open connection popup. */
void DeezerPlugin::login()
{
	qDebug() << Q_FUNC_INFO;
	WebView *webView = new WebView;
	webView->installEventFilter(this);
	webView->loadUrl(QUrl("https://connect.deezer.com/oauth/auth.php?app_id=141475&redirect_uri=http://www.miam-player.org/deezer-micro/channel.html&response_type=token&scope=manage_library,basic_access"));
	webView->show();
	_pages.append(webView);
}

/** Save credentials in the registry. */
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

/** Search for expression (artist, album, etc). */
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
	// NetworkAccessManager::getInstance()->get(QNetworkRequest(QUrl("http://api.deezer.com/search/artist/?output=xml&limit=5&q=" + expr)));
	NetworkAccessManager::getInstance()->get(QNetworkRequest(QUrl("http://api.deezer.com/search/album/?output=xml&limit=5&q=" + expr)));
	// NetworkAccessManager::getInstance()->get(QNetworkRequest(QUrl("http://api.deezer.com/search/track/?output=xml&limit=5&q=" + expr)));
}

void DeezerPlugin::searchRequestFinished(QXmlStreamReader &xml)
{
	QList<QStandardItem*> results;

	while(!xml.atEnd() && !xml.hasError()) {

		QXmlStreamReader::TokenType token = xml.readNext();
		if (token == QXmlStreamReader::StartElement) {
			if (xml.name() == "album") {
				QString id = this->extract(xml, "id");
				QString element = this->extract(xml, "title");
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

void DeezerPlugin::updateCacheDatabase(const std::list<RemoteTrack> &tracks)
{
	qDebug() << Q_FUNC_INFO;
	if (!_db.isOpen()) {
		_db.open();
	}

	for (std::list<RemoteTrack>::const_iterator it = tracks.cbegin(); it != tracks.cend(); ++it) {
		RemoteTrack track = *it;
		QSqlQuery insert(_db);
		insert.prepare("INSERT INTO tracks (id, name) VALUES (?, ?)");
		insert.addBindValue(track.id());
		insert.addBindValue(track.title());
		if (insert.exec()) {
			qDebug() << track.title() << "has been added to local deezer cache";
		}
	}
	_db.close();
}

void DeezerPlugin::trackWasDoubleClicked(const QModelIndex &index)
{
	QStandardItemModel *m = qobject_cast<QStandardItemModel*>(_tracks->model());
	if (m->itemFromIndex(index)->data(AbstractSearchDialog::DT_Origin) == _checkBox->text()) {
		qDebug() << Q_FUNC_INFO << "not implemented";
		qDebug() << m->itemFromIndex(index)->text();
	}
}
