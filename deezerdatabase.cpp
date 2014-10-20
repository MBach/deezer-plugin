#include "deezerdatabase.h"

#include "settings.h"

#include <QDir>
#include <QFile>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStandardPaths>

#include <QtDebug>

DeezerDatabase::DeezerDatabase()
	: QSqlDatabase("QSQLITE")
{
	Settings *settings = Settings::getInstance();
	QString path("%1/%2/%3");
	path = path.arg(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation),
					settings->organizationName(),
					settings->applicationName());
	QString dbPath = QDir::toNativeSeparators(path + "/deezer_cache.db");

	// Init a new database file for settings
	QFile db(dbPath);
	_isEmpty = !db.exists();
	db.open(QIODevice::ReadWrite);
	db.close();
	setDatabaseName(dbPath);

	if (open()) {
		exec("PRAGMA synchronous = OFF");
		exec("PRAGMA journal_mode = MEMORY");
		exec("PRAGMA temp_store = MEMORY");
		exec("CREATE TABLE IF NOT EXISTS artists (dzId INTEGER, name varchar(255), normalizedId INTEGER, normalizedName varchar(255))");
		exec("CREATE TABLE IF NOT EXISTS albums (dzId INTEGER, name varchar(255), normalizedId INTEGER, normalizedName varchar(255), year INTEGER, cover varchar(255), artist varchar(255))");
		QString createTableTracks = "CREATE TABLE IF NOT EXISTS tracks (trackNumber INTEGER, dzId INTEGER, name varchar(255), " \
			"artist varchar(255), album varchar(255), length INTEGER, rating INTEGER, year INTEGER, disc INTEGER)";
		exec(createTableTracks);
		close();
	}
}

void DeezerDatabase::extractTo(SqlDatabase *db)
{
	if (!isOpen()) {
		open();
	}
	qDebug() << Q_FUNC_INFO;

	// QString selectArtists = "SELECT t.trackNumber, t.name, al.name AS album, t.length, ar.name AS artist, t.rating, al.year AS year, t.id "


	QString selectAll = "SELECT t.trackNumber, t.name, al.name AS album, t.length, ar.name AS artist, t.rating, al.year AS year, t.id " \
		"FROM tracks t " \
		"INNER JOIN albums al ON t.album = al.name " \
		"INNER JOIN artists ar ON t.artist = ar.name";
	QSqlQuery query(selectAll, *this);
	if (query.exec()) {
		QList<TrackDAO> tracks;
		while (query.next()) {
			QSqlRecord r = query.record();
			TrackDAO track;
			int i = -1;
			track.setTrackNumber(r.value(++i).toString());
			track.setTitle(r.value(++i).toString());
			track.setAlbum(r.value(++i).toString());
			track.setLength(r.value(++i).toString());
			track.setArtist(r.value(++i).toString());
			track.setRating(r.value(++i).toInt());
			track.setYear(r.value(++i).toString());
			track.setUri("http://www.deezer.com/track/" + r.value(++i).toString());
			track.setIconPath(":/deezer-icon2");
			tracks.append(track);
		}
		db->loadRemoteTracks(tracks);
	}
	close();
}

/*void DeezerDatabase::updateTableTracks(const std::list<TrackDAO> &tracks)
{
	if (!isOpen()) {
		open();
	}

	transaction();
	for (std::list<TrackDAO>::const_iterator it = tracks.cbegin(); it != tracks.cend(); ++it) {
		TrackDAO track = *it;
		QSqlQuery insert(*this);
		insert.prepare("INSERT INTO tracks (trackNumber, id, name, album, length, artist, rating, year, disc) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");
		insert.addBindValue(track.trackNumber());
		insert.addBindValue(track.id());
		insert.addBindValue(track.title());
		insert.addBindValue(track.album());
		insert.addBindValue(track.length());
		insert.addBindValue(track.artist());
		insert.addBindValue(track.rating());
		insert.addBindValue(track.year());
		insert.addBindValue(track.disc());
		insert.exec();
	}
	commit();
	close();
}*/
