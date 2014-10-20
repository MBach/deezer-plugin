#include "deezerplugin.h"
#include "settings.h"
#include "webview.h"
#include "cookiejar.h"
#include "networkaccessmanager.h"
#include "abstractsearchdialog.h"
#include "model/playlistdao.h"

#include <QDesktopServices>
#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStandardPaths>
#include <QWebView>

#include <QtDebug>

/** Default constructor. */
DeezerPlugin::DeezerPlugin()
	: QObject(), _artists(NULL), _albums(NULL), _tracks(NULL), _searchDialog(NULL),
	  _checkBox(NULL), _webPlayer(new DeezerWebPlayer(this)), _db(NULL)
{
	NetworkAccessManager *nam = NetworkAccessManager::getInstance();
	nam->setCookieJar(new CookieJar);

	// Dispatch replies: search for something, get artist info, get tracks from album, get track info, fetch, synchronize...
	connect(nam, &QNetworkAccessManager::finished, this, &DeezerPlugin::dispatchReply);
	connect(_webPlayer->webView(), &WebView::aboutToSyncWithToken, this, &DeezerPlugin::sync);

	QWebSettings *s = QWebSettings::globalSettings();
	/// XXX
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
void DeezerPlugin::setDatabase(SqlDatabase *db)
{
	_db = db;
	NetworkAccessManager *inst = NetworkAccessManager::getInstance();
	connect(inst, &NetworkAccessManager::syncHasFinished, this, [=]() {
		// _dzDb.extractTo(_db);
	});
}

/** Redefined. */
void DeezerPlugin::sync(const QString &token) const
{
	if (!token.isEmpty()) {
		qDebug() << Q_FUNC_INFO << token;
		qDebug() << "Ready to synchronize Library with Deezer (fetching remote only)";
		// First, get checksum for Artists, Albums and Playlists
		/// TODO: albums (followers?)
		NetworkAccessManager *inst = NetworkAccessManager::getInstance();
		inst->get(QNetworkRequest(QUrl("http://api.deezer.com/user/me/artists?output=xml&access_token=" + token)));
		inst->get(QNetworkRequest(QUrl("http://api.deezer.com/user/me/playlists?output=xml&access_token=" + token)));
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

void DeezerPlugin::extractAlbumListFromArtist(QNetworkReply *reply, const QString &dzArtistId, QXmlStreamReader &xml)
{
	qDebug() << Q_FUNC_INFO;
	QList<AlbumDAO*> albums;
	Settings *settings = Settings::getInstance();
	qDebug() << "syncOptions" << settings->value("DeezerPlugin/syncOptions").toStringList();
	while(!xml.atEnd() && !xml.hasError()) {
		QXmlStreamReader::TokenType token = xml.readNext();
		if (token == QXmlStreamReader::StartElement) {
			if (xml.name() == "album") {
				AlbumDAO *album = new AlbumDAO;
				QString record_type;
				album->setId(this->extract(xml, "id"));
				QString title = this->extract(xml, "title");
				// 7 == length(' (YYYY)')
				album->setTitle(title.left(title.length() - 7));
				album->setYear(title.mid(title.length() - 5, 4));
				record_type = this->extract(xml, "record_type");
				if (settings->value("DeezerPlugin/syncOptions").toStringList().contains(record_type)) {
					qDebug() << "title of album to append" << title;
					albums.append(album);
				} else {
					qDebug() << "title of album to exclude" << title;
				}
			}
		}
	}
	QString artist = reply->property("artist").toString();
	uint artistId = qHash(_db->normalizeField(artist));
	for (int i = 0; i < albums.size(); i++) {
		AlbumDAO *album = albums.at(i);
		album->setArtist(artist);
		bool ok = _db->insertIntoTableAlbums(artistId, *album);

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

	QString playlistImage = QDir::toNativeSeparators(_cachePath.absolutePath() + "/playlist_" + playlistId + ".jpg");
	img.save(playlistImage);

	_db->updateTablePlaylistWithBackgroundImage(playlistId.toInt(), playlistImage);
}

void DeezerPlugin::extractImageCoverForLibrary(const QUrl &url, const QVariant &va, QByteArray &ba)
{
	qDebug() << Q_FUNC_INFO << url << va;

	if (va.canConvert<AlbumDAO>()) {
		/// XXX: magic number
		QString u = url.toString();
		int slash = u.indexOf("/", 28);
		QString albumId = u.mid(28, slash - 28);
		QImage img = QImage::fromData(ba);

		QString coverImage = QDir::toNativeSeparators(_cachePath.absolutePath() + "/album_" + albumId + ".jpg");
		img.save(coverImage);

		AlbumDAO album = va.value<AlbumDAO>();
		_db->updateTableAlbumWithCoverImage(album.id().toInt(), coverImage);
	}
}

void DeezerPlugin::extractSynchronizedArtists(QXmlStreamReader &xml)
{
	qDebug() << Q_FUNC_INFO;
	Settings *settings = Settings::getInstance();
	bool needToSyncArtists = false;
	QVariant checkSum = settings->value("DeezerPlugin/artists/checksum");
	QString sum;
	QList<ArtistDAO> artists;
	while(!xml.atEnd() && !xml.hasError()) {
		QXmlStreamReader::TokenType token = xml.readNext();
		if (token == QXmlStreamReader::StartElement) {
			if (xml.name() == "artist") {
				ArtistDAO artist;
				artist.setId(this->extract(xml, "id"));
				artist.setTitle(this->extract(xml, "name"));
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
		_db->removeRecordsFromHost(this->name());
		bool ok = true;
		for (int i = 0; i < artists.size(); i++) {
			ArtistDAO artist = artists.at(i);
			if (_db->insertIntoTableArtists(artist)) {
				QNetworkRequest r(QUrl("http://api.deezer.com/artist/" + artist.id() + "/albums?output=xml"));
				QNetworkReply *reply = NetworkAccessManager::getInstance()->get(r);
				reply->setProperty("artist", artist.title());
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
	Settings *settings = Settings::getInstance();
	bool needToSyncPlaylists = false;
	QVariant checkSum = settings->value("DeezerPlugin/playlists/checksum");
	QString sum;
	QList<PlaylistDAO> playlists;
	while(!xml.atEnd() && !xml.hasError()) {
		QXmlStreamReader::TokenType token = xml.readNext();
		if (token == QXmlStreamReader::StartElement) {
			if (xml.name() == "playlist") {
				PlaylistDAO playlist;
				playlist.setId(this->extract(xml, "id"));
				playlist.setTitle(this->extract(xml, "title"));
				playlist.setLength(this->extract(xml, "duration"));

				// Extract picture in another request
				QString picture = this->extract(xml, "picture");
				if (!picture.isEmpty() && !_db->playlistHasBackgroundImage(playlist.id().toInt())) {
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
		_db->removePlaylists(playlists);
		bool ok = true;
		for (int i = 0; i < playlists.size(); i++) {
			PlaylistDAO playlist = playlists.at(i);
			playlist.setIcon(":/deezer-icon");

			// Forward tracklist
			if (_db->insertIntoTablePlaylists(playlist) > 0) {
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
				track.setIconPath(":/deezer-icon");
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
	_db->insertIntoTablePlaylistTracks(playlistId.toInt(), tracks);
}

void DeezerPlugin::extractTrackListFromAlbum(QNetworkReply *reply, const QString &dzAlbumId, QXmlStreamReader &xml)
{
	std::list<TrackDAO> tracks;
	if (_cache.contains(dzAlbumId)) {

		AlbumDAO *cachedAlbum = _cache.value(dzAlbumId);
		qDebug() << "cachedAlbum" << cachedAlbum;
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
					track.setIconPath(":/deezer-icon");
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
			qDebug() << "removing reply" << reply->url();
			_repliesWhichInteractWithUi.remove(reply);
			delete reply;
		} else {
			_db->insertIntoTableTracks(tracks);
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
	QUrl redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
	if (!redirectUrl.isEmpty() && redirectUrl != reply->request().url()) {
		QNetworkReply *forward = NetworkAccessManager::getInstance()->get(QNetworkRequest(redirectUrl));
		forward->setProperty("origin", reply->request().url());
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
	} else if (r.startsWith("http://api.deezer.com/user/me/artists")) {
		this->extractSynchronizedArtists(xml);
	/// TODO !?
	//} else if (r.startsWith("http://api.deezer.com/user/me/albums")) {
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
