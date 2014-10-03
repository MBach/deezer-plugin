#ifndef DEEZERPLUGIN_H
#define DEEZERPLUGIN_H

#include "miamcore_global.h"
#include "remotemediaplayerplugin.h"
#include "model/trackdao.h"

#include <QCache>
#include <QNetworkReply>
#include <QXmlStreamReader>
#include "ui_config.h"
#include "deezerdatabase.h"
#include "deezerwebplayer.h"
#include "model/sqldatabase.h"

class QWebView;

/**
 * \brief       Deezer plugin
 * \author      Matthieu Bachelier
 * \version     0.2
 * \copyright   GNU General Public License v3
 */
class DeezerPlugin : public QObject, public RemoteMediaPlayerPlugin
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID RemoteMediaPlayerPlugin_iid)
	Q_INTERFACES(RemoteMediaPlayerPlugin)
	Q_ENUMS(Reply)

public:
	enum Reply { RPL_SendToCurrentPlaylist = 0,
				 RPL_UpdateCacheDatabase = 1};

private:
	Ui::DeezerPluginConfigPage _config;
	AbstractSearchDialog* _searchDialog;
	QList<QWebView*> _pages;
	QListView *_artists;
	QListView *_albums;
	QListView *_tracks;
	QCheckBox *_checkBox;
	DeezerWebPlayer *_webPlayer;

	DeezerDatabase _dzDb;
	SqlDatabase _db;

	QMap<QString, TrackDAO*> _cache;
	QMap<QNetworkReply*, Reply> _repliesWhichInteractWithUi;
	QList<QNetworkReply*> _pendingRequest;

public:
	/** Default constructor. */
	explicit DeezerPlugin();

	/** Default destructor. */
	virtual ~DeezerPlugin();

	/** Load and return user interface to manipulate this plugin. */
	virtual QWidget* configPage();

	/** This plugin can be configurable in options. */
	inline virtual bool isConfigurable() const { return true; }

	/** Name displayed in options. */
	inline virtual QString name() const { return "Deezer-Plugin"; }

	/** Every RemoteMediaPlayerPlugin has to return an implementation of RemoteMediaPlayer class. */
	inline virtual RemoteMediaPlayer * player() const { return _webPlayer;	}

	/** This plugin has no independant view. */
	inline virtual QWidget* providesView() { return NULL; }

	/** Release displayed in options. */
	inline virtual QString version() const { return "0.2"; }

	/** Redefined. */
	virtual void setSearchDialog(AbstractSearchDialog *w);

	/** Redefined. */
	virtual void sync(const QString &token) const;

protected:
	/** Redefined to open Deezer's home page. */
	bool eventFilter(QObject *obj, QEvent *event);

private:
	QString extract(QXmlStreamReader &xml, const QString &criterion);
	void extractAlbum(QNetworkReply *reply, QXmlStreamReader &xml);
	void extractAlbumListFromArtist(QNetworkReply *reply, const QString &artistId, QXmlStreamReader &xml);
	void extractSynchronizedArtists(QXmlStreamReader &xml);
	void extractSynchronizedPlaylists(QXmlStreamReader &xml);
	void extractSynchronizedTracksFromPlaylists(const QString &playlistId, QXmlStreamReader &xml, int index = 0);
	void extractTrackListFromAlbum(QNetworkReply *reply, const QString &albumID, QXmlStreamReader &xml);
	void searchRequestFinished(QXmlStreamReader &xml);
	void updateTableTracks(const std::list<TrackDAO> &tracks);

private slots:
	/** TODO. */
	void artistWasDoubleClicked(const QModelIndex &index);

	/** Fetch detailed information about the Album and append it to playlist. */
	void albumWasDoubleClicked(const QModelIndex &index);

	/** Display everything! */
	void dispatchReply(QNetworkReply *reply);

	/** Open connection popup. */
	void login();

	/** Save credentials in the registry. */
	void saveCredentials(bool enabled);

	/** Search for expression (artist, album, etc). */
	void search(const QString &expr);

	/** TODO. */
	void trackWasDoubleClicked(const QModelIndex &index);

signals:
	/** Callback for the view. */
	void searchComplete(AbstractSearchDialog::Request, QList<QStandardItem*> results);

	void aboutToProcessRemoteTracks(const std::list<TrackDAO> &tracks);
};

#endif // DEEZERPLUGIN_H
