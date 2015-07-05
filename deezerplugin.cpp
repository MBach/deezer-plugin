#include "deezerplugin.h"
#include "settings.h"
#include "webview.h"
#include "cookiejar.h"
#include "networkaccessmanager.h"
#include "abstractsearchdialog.h"
#include "model/playlistdao.h"
#include "model/sqldatabase.h"

#include <QDesktopServices>
#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStandardPaths>

#include <QtDebug>

/** Default constructor. */
DeezerPlugin::DeezerPlugin()
	: QObject(), _artists(nullptr), _albums(nullptr), _tracks(nullptr), _searchDialog(nullptr),
	  _checkBox(nullptr), _webPlayer(new DeezerWebPlayer(this))
{
	NetworkAccessManager *nam = NetworkAccessManager::getInstance();
	nam->setCookieJar(new CookieJar);

	// Dispatch replies: search for something, get artist info, get tracks from album, get track info, fetch, synchronize...
	connect(nam, &QNetworkAccessManager::finished, this, &DeezerPlugin::dispatchReply);
	// connect(nam, &NetworkAccessManager::syncHasFinished, SqlDatabase::instance(), &SqlDatabase::load);
	connect(_webPlayer->webView(), &WebView::aboutToSyncWithToken, this, &DeezerPlugin::sync);

	QWebSettings *s = QWebSettings::globalSettings();
	/// XXX
	// s->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
	s->setAttribute(QWebSettings::PluginsEnabled, true);
	s->setAttribute(QWebSettings::JavascriptEnabled, true);
	s->setAttribute(QWebSettings::JavascriptCanOpenWindows, true);
	s->setAttribute(QWebSettings::JavascriptCanCloseWindows, true);
	s->setAttribute(QWebSettings::DnsPrefetchEnabled, true);
	s->setAttribute(QWebSettings::LocalStorageEnabled, true);
	s->setAttribute(QWebSettings::LocalContentCanAccessRemoteUrls, true);
	s->setAttribute(QWebSettings::LocalContentCanAccessFileUrls, true);
	s->setThirdPartyCookiePolicy(QWebSettings::AlwaysAllowThirdPartyCookies);

	Settings *settings = Settings::instance();
	if (settings->value("DeezerPlugin/syncOptions").isNull()) {
		QStringList l = QStringList() << "album" << "ep";
		settings->setValue("DeezerPlugin/syncOptions", l);
	}

	QString path("%1/%2/%3/cache");
	path = path.arg(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation),
					settings->organizationName(),
					settings->applicationName());
	_cachePath = QDir(path);
	if (!_cachePath.exists()) {
		_cachePath.mkpath(path);
	}

	auto db = SqlDatabase::instance();
	connect(db, &SqlDatabase::aboutToResyncRemoteSources, this, [=]() {
		qDebug() << "aboutToResyncRemoteSources" << _token;
		settings->remove("DeezerPlugin/artists/checksum");
		sync(_token);
	});
}

/** Default destructor. */
DeezerPlugin::~DeezerPlugin()
{
	qDebug() << Q_FUNC_INFO;
	for (QWebView *view : _pages) {
		delete view;
		view = nullptr;
	}
	_pages.clear();
	if (_checkBox) {
		delete _checkBox;
	}
}

void DeezerPlugin::cleanUpBeforeDestroy()
{
	SqlDatabase *db = SqlDatabase::instance();
	db->removeRecordsFromHost(_webPlayer->host());
	Settings *settings = Settings::instance();
	settings->remove("DeezerPlugin/albums/checksum");
	settings->remove("DeezerPlugin/artists/checksum");

	db->removePlaylistsFromHost(_webPlayer->host());
	settings->remove("DeezerPlugin/playlists/checksum");
}

/** Load and return user interface to manipulate this plugin. */
QWidget* DeezerPlugin::configPage()
{
	Settings *settings = Settings::instance();
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
	connect(this, &DeezerPlugin::searchCompleted, w, &AbstractSearchDialog::processResults);
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
	qDebug() << Q_FUNC_INFO << token;
	if (!token.isEmpty() || !_token.isEmpty()) {
		QString t;
		if (token.isEmpty()) {
			t = _token;
		} else {
			_token = token;
			t = token;
		}
		// Ready to synchronize Library with Deezer (fetching remote only)";
		// First, get checksum for Artists, Albums and Playlists
		/// TODO: albums (followers?)
		NetworkAccessManager *inst = NetworkAccessManager::getInstance();
		inst->get(QNetworkRequest(QUrl("http://api.deezer.com/user/me/albums?output=xml&access_token=" + t)));
		inst->get(QNetworkRequest(QUrl("http://api.deezer.com/user/me/artists?output=xml&access_token=" + t)));
		inst->get(QNetworkRequest(QUrl("http://api.deezer.com/user/me/playlists?output=xml&access_token=" + t)));
		inst->setSync(true);
	}
}

/** Redefined to open Deezer's home page. */
bool DeezerPlugin::eventFilter(QObject *obj, QEvent *event)
{
	if (obj == _config.logo && event->type() == QEvent::MouseButtonPress) {
		QDesktopServices::openUrl(QUrl("http://www.deezer.com"));
		return true;
	} else if (event->type() == QEvent::Show) {
		return true;
	} else {
		return QObject::eventFilter(obj, event);
	}
}

QString DeezerPlugin::extract(QXmlStreamReader &xml, const QString &criterion)
{
	while (xml.name() != criterion && !xml.hasError()) {
		QXmlStreamReader::TokenType t = xml.readNext();
		//if (!isEager && t == QXmlStreamReader::EndElement) {
		//	break;
		//}
	}
	return xml.readElementText();
}

void DeezerPlugin::extractAlbum(QNetworkReply *reply, QXmlStreamReader &xml)
{
	bool artistFound = false;
	bool albumFound = false;
	bool albumIdFound = false;
	QString id;
	QString artist;
	QString album;
	QString year;
	AlbumDAO *albumDAO = new AlbumDAO;
	while (!xml.atEnd() && !xml.hasError()) {

		QXmlStreamReader::TokenType token = xml.readNext();

		// Parse start elements
		if (token == QXmlStreamReader::StartElement) {

			if (!albumIdFound) {
				id = this->extract(xml, "id");
				albumIdFound = true;
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
				albumDAO->setId(id);
				albumDAO->setTitle(album);
				albumDAO->setArtist(artist);
				albumDAO->setYear(year);
				_cache.insert(id, albumDAO);

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

void DeezerPlugin::extractAlbumListFromArtist(QNetworkReply *reply, const QString &, QXmlStreamReader &xml)
{
	QList<AlbumDAO*> albums;
	Settings *settings = Settings::instance();
	while(!xml.atEnd() && !xml.hasError()) {
		QXmlStreamReader::TokenType token = xml.readNext();
		if (token == QXmlStreamReader::StartElement) {
			if (xml.name() == "album") {
				AlbumDAO *album = new AlbumDAO;
				QString record_type;
				album->setId(this->extract(xml, "id"));
				album->setTitle(this->extract(xml, "title"));
				//QString releaseDate = this->extract(xml, "release_date");
				//album->setYear(releaseDate.left(4));
				record_type = this->extract(xml, "record_type");
				if (settings->value("DeezerPlugin/syncOptions").toStringList().contains(record_type)) {
					albums.append(album);
				}
			}
		}
	}
	QString artist = reply->property("artist").toString();
	SqlDatabase *db = SqlDatabase::instance();
	uint artistId = qHash(db->normalizeField(artist));
	for (int i = 0; i < albums.size(); i++) {
		AlbumDAO *album = albums.at(i);
		album->setArtist(artist);
		album->setHost(_webPlayer->host());
		album->setIcon(":/deezer-icon2");
		ArtistDAO *artistDao = static_cast<ArtistDAO*>(_cache.value(QString::number(artistId)));
		if (artistDao) {
			album->setParentNode(artistDao);
		}
		bool ok = db->insertIntoTableAlbums(artistId, album);

		// Finally, extract track list
		/// XXX: too much requests?
		if (ok) {
			album->setArtist(artist);
			_cache.insert(album->id(), album);

			QNetworkReply *nextReply = NetworkAccessManager::getInstance()->get(QNetworkRequest(QUrl("http://api.deezer.com/album/" + album->id() + "/tracks/?output=xml")));
			if (_repliesWhichInteractWithUi.contains(reply)) {
				Reply forward =_repliesWhichInteractWithUi.take(reply);
				delete reply;
				_repliesWhichInteractWithUi.insert(nextReply, forward);
			}

			QNetworkRequest r(QUrl("http://api.deezer.com/album/" + album->id() + "/image?size=big"));
			QNetworkReply *getCover = NetworkAccessManager::getInstance()->get(r);
			getCover->setProperty("albumDAO", QVariant::fromValue(*album));
			_pendingRequest.append(getCover);
		}
	}
}

void DeezerPlugin::extractImageForPlaylist(const QUrl &url, QByteArray &ba)
{
	/// XXX: magic number
	QString u = url.toString();
	int slash = u.indexOf("/", 31);
	QString playlistId = u.mid(31, slash - 31);
	QImage img = QImage::fromData(ba);

	QString playlistImage = _cachePath.absolutePath() + "/playlist_" + playlistId + ".jpg";
	img.save(playlistImage);

	SqlDatabase::instance()->updateTablePlaylistWithBackgroundImage(playlistId.toUInt(), playlistImage);
}

void DeezerPlugin::extractImageCoverForLibrary(const QUrl &url, const QVariant &va, QByteArray &ba)
{
	if (va.canConvert<AlbumDAO>()) {
		/// XXX: magic number
		QString u = url.toString();
		int slash = u.indexOf("/", 28);
		QString albumId = u.mid(28, slash - 28);
		QImage img = QImage::fromData(ba);

		QString coverImage = _cachePath.absolutePath() + "/album_" + albumId + ".jpg";
		img.save(coverImage);

		AlbumDAO album = va.value<AlbumDAO>();
		SqlDatabase::instance()->updateTableAlbumWithCoverImage(coverImage, album.title(), album.artist());
	}
}

void DeezerPlugin::extractSynchronizedAlbums(QXmlStreamReader &xml)
{
	qDebug() << Q_FUNC_INFO;
	Settings *settings = Settings::instance();
	bool needToSyncAlbums = false;
	QVariant checkSum = settings->value("DeezerPlugin/albums/checksum");
	QString sum;
	QList<AlbumDAO*> albums;
	while(!xml.atEnd() && !xml.hasError()) {
		QXmlStreamReader::TokenType token = xml.readNext();
		if (token == QXmlStreamReader::StartElement) {
			if (xml.name() == "artist") {
				AlbumDAO *album = new AlbumDAO;
				album->setId(this->extract(xml, "id"));
				album->setTitle(this->extract(xml, "title"));
				album->setCover(this->extract(xml, "cover"));
				album->setYear(this->extract(xml, "release_date").left(4));
				//this->extract(xml, "artist");
				album->setArtist(this->extract(xml, "name"));
				album->setHost(_webPlayer->host());
				albums.append(albums);
				qDebug() << "new album to sync" << album->id() << album->title() << album->cover() << album->year() << album->artist();
			}
			if (xml.name() == "checksum") {
				sum = xml.readElementText();
				// Out of sync
				if (checkSum.isNull() || checkSum.toString() != sum) {
					needToSyncAlbums = true;
					qDebug() << "needToSyncAlbums" << needToSyncAlbums;
				}
			}
		}
	}
	if (needToSyncAlbums) {
		SqlDatabase *db = SqlDatabase::instance();
		//db->removeRecordsFromHost(_webPlayer->host());
		bool ok = true;
		for (int i = 0; i < albums.size(); i++) {
			AlbumDAO *album = albums.at(i);
			qDebug() << "inserting new album in cache" << album->title();
			uint artistId = qHash(db->normalizeField(album->artist()));
			_cache.insert(album->id(), album);
			if (db->insertIntoTableAlbums(artistId, album)) {

				QNetworkReply *nextReply = NetworkAccessManager::getInstance()->get(QNetworkRequest(QUrl("http://api.deezer.com/album/" + album->id() + "/tracks/?output=xml")));
				_repliesWhichInteractWithUi.insert(nextReply, RPL_UpdateCacheDatabase);

			} else {
				ok = false;
			}
		}
		if (ok) {
			settings->setValue("DeezerPlugin/albums/checksum", sum);
		}
	} else {
		qDebug() << "albums are synchronized!";
	}
}

void DeezerPlugin::extractSynchronizedArtists(QXmlStreamReader &xml)
{
	qDebug() << Q_FUNC_INFO;
	Settings *settings = Settings::instance();
	bool needToSyncArtists = false;
	QVariant checkSum = settings->value("DeezerPlugin/artists/checksum");
	QString sum;
	QList<ArtistDAO*> artists;
	while(!xml.atEnd() && !xml.hasError()) {
		QXmlStreamReader::TokenType token = xml.readNext();
		if (token == QXmlStreamReader::StartElement) {
			if (xml.name() == "artist") {
				ArtistDAO *artist = new ArtistDAO;
				artist->setId(this->extract(xml, "id"));
				artist->setTitle(this->extract(xml, "name"));
				artist->setHost(_webPlayer->host());
				artists.append(artist);
			}
			if (xml.name() == "checksum") {
				sum = xml.readElementText();
				// Out of sync
				if (checkSum.isNull() || checkSum.toString() != sum) {
					needToSyncArtists = true;
					qDebug() << "needToSyncArtists" << needToSyncArtists;
				}
			}
		}
	}
	if (needToSyncArtists) {
		SqlDatabase *db = SqlDatabase::instance();
		db->removeRecordsFromHost(_webPlayer->host());
		bool ok = true;
		for (int i = 0; i < artists.size(); i++) {
			ArtistDAO *artist = artists.at(i);
			qDebug() << "inserting new artist in cache" << artist->title();
			_cache.insert(QString::number(qHash(db->normalizeField(artist->title()))), artist);
			if (db->insertIntoTableArtists(artist)) {
				QNetworkRequest r(QUrl("http://api.deezer.com/artist/" + artist->id() + "/albums?output=xml"));
				QNetworkReply *reply = NetworkAccessManager::getInstance()->get(r);
				reply->setProperty("artist", artist->title());
				_repliesWhichInteractWithUi.insert(reply, RPL_UpdateCacheDatabase);
			} else {
				ok = false;
			}
		}
		if (ok) {
			settings->setValue("DeezerPlugin/artists/checksum", sum);
		}
	} else {
		qDebug() << "artists are synchronized!";
	}
}

void DeezerPlugin::extractSynchronizedPlaylists(QXmlStreamReader &xml)
{
	qDebug() << Q_FUNC_INFO;
	Settings *settings = Settings::instance();
	bool needToSyncPlaylists = false;
	QVariant checkSum = settings->value("DeezerPlugin/playlists/checksum");
	QString sum;
	QList<PlaylistDAO> playlists;
	SqlDatabase *db = SqlDatabase::instance();
	while(!xml.atEnd() && !xml.hasError()) {
		QXmlStreamReader::TokenType token = xml.readNext();
		if (token == QXmlStreamReader::StartElement) {
			if (xml.name() == "playlist") {
				PlaylistDAO playlist;
				playlist.setId(this->extract(xml, "id"));
				playlist.setTitle(this->extract(xml, "title"));
				playlist.setLength(this->extract(xml, "duration"));
				playlist.setHost(_webPlayer->host());

				// Extract picture in another request
				QString picture = this->extract(xml, "picture");
				if (!picture.isEmpty() && !db->playlistHasBackgroundImage(playlist.id().toUInt())) {
					qDebug() << "picture was found and no bg was found in database";
					QNetworkRequest r(QUrl("http://api.deezer.com/playlist/" + playlist.id() + "/image?size=big"));
					QNetworkReply *reply = NetworkAccessManager::getInstance()->get(r);
					_pendingRequest.append(reply);
				}

				playlist.setChecksum(this->extract(xml, "checksum"));
				playlists.append(playlist);
			} else if (xml.name() == "checksum") {
				sum = xml.readElementText();
				// Out of sync
				if (checkSum.isNull() || checkSum.toString() != sum) {
					needToSyncPlaylists = true;
				}
			}
		}
	}
	if (needToSyncPlaylists) {
		for (PlaylistDAO dao : playlists) {
			db->removePlaylist(dao.id().toUInt());
		}
		bool ok = true;
		for (int i = 0; i < playlists.size(); i++) {
			PlaylistDAO playlist = playlists.at(i);
			playlist.setIcon(":/deezer-icon");

			// Forward tracklist
			/// FIXME
			std::list<TrackDAO> tracks;
			if (db->insertIntoTablePlaylists(playlist, tracks, false) > 0) {
				QNetworkRequest r(QUrl("http://api.deezer.com/playlist/" + playlist.id() + "/tracks?output=xml"));
				QNetworkReply *reply = NetworkAccessManager::getInstance()->get(r);
				_pendingRequest.append(reply);
			} else {
				ok = false;
			}
		}
		if (ok) {
			settings->setValue("DeezerPlugin/playlists/checksum", sum);
		}
	} else {
		qDebug() << "playlists are synchronized!";
	}
}

void DeezerPlugin::extractSynchronizedTracksFromPlaylists(const QString &playlistId, QXmlStreamReader &xml, int index)
{
	qDebug() << Q_FUNC_INFO;
	std::list<TrackDAO> tracks;
	while (!xml.atEnd() && !xml.hasError()) {
		QXmlStreamReader::TokenType token = xml.readNext();
		if (token == QXmlStreamReader::StartElement) {
			if (xml.name() == "track") {
				TrackDAO track;
				track.setId(this->extract(xml, "id"));
				track.setTitle(this->extract(xml, "title"));
				track.setUri(this->extract(xml, "link"));
				track.setLength(this->extract(xml, "duration"));

				// in node <artist></artist>
				track.setArtist(this->extract(xml, "name"));

				// in node <album></album>
				track.setAlbum(this->extract(xml, "title"));
				track.setIcon(":/deezer-icon");
				track.setHost(_webPlayer->host());
				tracks.push_back(std::move(track));
			} else if (xml.name() == "next") {
				if (index == 0) {
					index += 50;
				}
				QString url = "http://api.deezer.com/playlist/" + playlistId + "/tracks/?output=xml&index=" + QString::number(index);
				NetworkAccessManager::getInstance()->get(QNetworkRequest(QUrl(url)));
			}
		}
	}
	SqlDatabase::instance()->insertIntoTablePlaylistTracks(playlistId.toUInt(), tracks);
}

void DeezerPlugin::extractTrackListFromAlbum(QNetworkReply *reply, const QString &dzAlbumId, QXmlStreamReader &xml)
{
	std::list<TrackDAO> tracks;
	if (_cache.contains(dzAlbumId)) {

		AlbumDAO *cachedAlbum = static_cast<AlbumDAO*>(_cache.value(dzAlbumId));
		qDebug() << "cachedAlbum" << cachedAlbum->title();
		while (!xml.atEnd() && !xml.hasError()) {
			QXmlStreamReader::TokenType token = xml.readNext();
			if (token == QXmlStreamReader::StartElement) {
				if (xml.name() == "track") {
					TrackDAO track;
					track.setId(this->extract(xml, "id"));
					track.setTitle(this->extract(xml, "title"));
					track.setUri(this->extract(xml, "link"));
					track.setLength(this->extract(xml, "duration"));
					track.setTrackNumber(this->extract(xml, "track_position"));
					track.setDisc(this->extract(xml, "disk_number"));
					int r = this->extract(xml, "rank").toInt();
					r = round((double)r / 166666);
					track.setRating(r);
					track.setArtist(cachedAlbum->artist());
					track.setAlbum(cachedAlbum->title());
					track.setHost(_webPlayer->host());
					track.setIcon(":/deezer-icon");
					track.setYear(cachedAlbum->year().left(4));
					tracks.push_back(std::move(track));
				}
			}
		}
		_cache.remove(dzAlbumId);

		/// XXX: this is really a big mess mixing updating cache and UI actions
		if (_repliesWhichInteractWithUi.contains(reply)) {
			Reply value = _repliesWhichInteractWithUi.value(reply);
			switch (value) {
			case RPL_SendToCurrentPlaylist:
				emit aboutToProcessRemoteTracks(tracks);
				break;

			//case RPL_UpdateCacheDatabase:
			//	this->updateCacheDatabase(tracks);
			//	break;
			}
			// qDebug() << "removing reply" << reply->url();
			_repliesWhichInteractWithUi.remove(reply);
			delete reply;
		} else {
			SqlDatabase::instance()->insertIntoTableTracks(tracks);
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

//#include <QWebFrame>

/** Display everything! */
void DeezerPlugin::dispatchReply(QNetworkReply *reply)
{
	//qDebug() << Q_FUNC_INFO << reply->url();
	QUrl redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
	if (!redirectUrl.isEmpty() && redirectUrl != reply->request().url()) {
		QNetworkReply *forward = NetworkAccessManager::getInstance()->get(QNetworkRequest(redirectUrl));
		forward->setProperty("origin", reply->request().url());
		forward->setProperty("albumDAO", reply->property("albumDAO"));
		return;
	}

	QNetworkRequest request = reply->request();
	QString r = request.url().toDisplayString();
	QByteArray ba = reply->readAll();
	QXmlStreamReader xml(ba);

	/// WARNING: used by sync() and search() -> album/<id>/tracklist
	QRegularExpression reg("http://api.deezer.com/album/[0-9]+/tracks");
	if (r.startsWith("http://api.deezer.com/album/") && !r.contains(reg)) {
		qDebug() << r;
		this->extractAlbum(reply, xml);
	} else if (r.startsWith("http://api.deezer.com/album/") && r.contains(reg)) {
		int slash = r.indexOf("/", 28);
		this->extractTrackListFromAlbum(reply, r.mid(28, slash - 28), xml);
	} else if (r.startsWith("http://api.deezer.com/user/me/albums")) {
		this->extractSynchronizedAlbums(xml);
	} else if (r.startsWith("http://api.deezer.com/user/me/artists")) {
		this->extractSynchronizedArtists(xml);
	} else if (r.startsWith("http://api.deezer.com/user/me/playlists")) {
		this->extractSynchronizedPlaylists(xml);
	} else if (r.startsWith("http://api.deezer.com/playlist/")) {
		int slash = r.indexOf("/", 31);
		QString playlistId = r.mid(31, slash - 31);
		int index = 0;
		if (r.contains("&index=")) {
			int equal = r.lastIndexOf("=");
			index = r.mid(equal + 1).toInt();
		}
		this->extractSynchronizedTracksFromPlaylists(playlistId, xml, index);
	} else if (r.startsWith("http://cdn-images.deezer.com/images/playlist")) {

		this->extractImageForPlaylist(reply->property("origin").toUrl(), ba);

	} else if (r.startsWith("http://cdn-images.deezer.com/images/cover")) {

		this->extractImageCoverForLibrary(reply->property("origin").toUrl(), reply->property("albumDAO"), ba);

	//} else if (r.startsWith("http://api.deezer.com/user/me/flow")) {
	//} else if (r.startsWith("http://api.deezer.com/user/me/following")) {
	//} else if (r.startsWith("http://api.deezer.com/user/me/followers")) {
	//} else if (r.startsWith("http://api.deezer.com/user/me/radio")) {
	//} else if (r.startsWith("http://api.deezer.com/user/me/tracks")) {
	} else if (r.startsWith("http://api.deezer.com/artist/")) {
		int slash = r.indexOf("/", 29);
		this->extractAlbumListFromArtist(reply, r.mid(29, slash - 29), xml);
	} else if (r.startsWith("http://api.deezer.com/search/artist/")) {
		/// TODO
	} else if (r.startsWith("http://api.deezer.com/search/album/")) {
		this->searchRequestFinished(xml);
	} else if (r.startsWith("http://api.deezer.com/search/track/")) {
		/// TODO
	} else if (r.startsWith("http://api.deezer.com/")) {
		qDebug() << "unknown search request" << r;
	} else if (r.startsWith("https://connect.deezer.com/login.php")) {
		/*qDebug() << "Connecting" << ba;
		// find fields "login_mail" "login_password"
		WebView *webView = _pages.first();
		if (webView) {
			webView->page()->mainFrame()->evaluateJavaScript("document.getElementById('login_mail').value = 'test'");
		}*/
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
	Settings *settings = Settings::instance();
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
					QStandardItem *item = new QStandardItem(QIcon(":/deezer-icon"), element);
					item->setData(_checkBox->text(), AbstractSearchDialog::DT_Origin);
					item->setData(id, AbstractSearchDialog::DT_Identifier);
					results.append(item);
				}
			}
		}
	}
	emit searchCompleted(AbstractSearchDialog::Album, results);
}

void DeezerPlugin::trackWasDoubleClicked(const QModelIndex &index)
{
	QStandardItemModel *m = qobject_cast<QStandardItemModel*>(_tracks->model());
	if (m->itemFromIndex(index)->data(AbstractSearchDialog::DT_Origin) == _checkBox->text()) {
		qDebug() << Q_FUNC_INFO << "not implemented";
		qDebug() << m->itemFromIndex(index)->text();
	}
}
