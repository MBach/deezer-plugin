#ifndef DEEZERPLUGIN_H
#define DEEZERPLUGIN_H

#include "miamcore_global.h"
#include "interfaces/remotemediaplayerplugin.h"
#include "model/trackdao.h"
#include "abstractsearchdialog.h"

#include <QCache>
#include <QDir>
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
 * \version     0.5
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
	mutable QString _token;
	Ui::DeezerPluginConfigPage _config;
	AbstractSearchDialog* _searchDialog;
	QList<WebView*> _pages;
	QListView *_artists;
	QListView *_albums;
	QListView *_tracks;
	QCheckBox *_checkBox;
	DeezerWebPlayer *_webPlayer;

	QMap<QString, GenericDAO*> _cache;
	QMap<QNetworkReply*, Reply> _repliesWhichInteractWithUi;
	QList<QNetworkReply*> _pendingRequest;
	QDir _cachePath;

public:
	/** Default constructor. */
	explicit DeezerPlugin();

	/** Default destructor. */
	virtual ~DeezerPlugin();

	virtual void cleanUpBeforeDestroy() override;

	/** Load and return user interface to manipulate this plugin. */
	virtual QWidget* configPage() override;

	/** This plugin can be configurable in options. */
	inline virtual bool isConfigurable() const { return true; }

	/** Name displayed in options. */
	inline virtual QString name() const { return "Deezer-Plugin"; }

	/** Every RemoteMediaPlayerPlugin has to return an implementation of RemoteMediaPlayer class. */
	inline virtual RemoteMediaPlayer * player() const { return _webPlayer;	}

	/** Release displayed in options. */
	inline virtual QString version() const { return "0.5"; }

	/** Redefined. */
	virtual void setSearchDialog(AbstractSearchDialog *w) override;

	/** Redefined. */
	virtual void sync(const QString &token) const override;

protected:
	/** Redefined to open Deezer's home page. */
	bool eventFilter(QObject *obj, QEvent *event) override;

private:
	QString extract(QXmlStreamReader &xml, const QString &criterion);
	void extractAlbum(QNetworkReply *reply, QXmlStreamReader &xml);
	void extractAlbumListFromArtist(QNetworkReply *reply, const QString &, QXmlStreamReader &xml);
	void extractImageForPlaylist(const QUrl &url, QByteArray &ba);
	void extractImageCoverForLibrary(const QUrl &url, const QVariant &va, QByteArray &ba);
	void extractSynchronizedAlbums(QXmlStreamReader &xml);
	void extractSynchronizedArtists(QXmlStreamReader &xml);
	void extractSynchronizedPlaylists(QXmlStreamReader &xml);
	void extractSynchronizedTracksFromPlaylists(const QString &playlistId, QXmlStreamReader &xml, int index = 0);
	void extractTrackListFromAlbum(QNetworkReply *reply, const QString &dzAlbumId, QXmlStreamReader &xml);
	void searchRequestFinished(QXmlStreamReader &xml);

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
	void searchCompleted(AbstractSearchDialog::Request, QList<QStandardItem*> results);

	void aboutToProcessRemoteTracks(const std::list<TrackDAO> &tracks);
};

#endif // DEEZERPLUGIN_H
