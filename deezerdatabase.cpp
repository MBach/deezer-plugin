#include "deezerdatabase.h"

#include "settings.h"

#include <QDir>
#include <QFile>
#include <QSqlQuery>
#include <QStandardPaths>

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
		exec("CREATE TABLE IF NOT EXISTS artists (id INTEGER PRIMARY KEY, name varchar(255))");
		exec("CREATE TABLE IF NOT EXISTS albums (id INTEGER PRIMARY KEY, name varchar(255), cover varchar(255), artist varchar(255))");
		exec("CREATE TABLE IF NOT EXISTS tracks (id INTEGER PRIMARY KEY, name varchar(255), artist varchar(255), album varchar(255))");
		exec("CREATE INDEX IF NOT EXISTS indexArtists ON artists (id)");
		exec("CREATE INDEX IF NOT EXISTS indexAlbums ON albums (id)");
		exec("CREATE INDEX IF NOT EXISTS indexTracks ON tracks (id)");
		close();
	}
}
