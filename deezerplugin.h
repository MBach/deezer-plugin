#ifndef DEEZERPLUGIN_H
#define DEEZERPLUGIN_H

#include "miamcore_global.h"
#include "remotemediaplayerplugin.h"
#include "model/remotetrack.h"

#include <QCache>
#include <QNetworkReply>
#include <QXmlStreamReader>
#include "ui_config.h"
#include "deezerdatabase.h"
#include "deezerwebplayer.h"

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
	DeezerDatabase _db;
	QCache<QString, RemoteTrack> _cache;
	QMap<QNetworkReply*, Reply> _repliesWhichInteractWithUi;

public:
	explicit DeezerPlugin();

	virtual ~DeezerPlugin();

	virtual QWidget* configPage();

	inline virtual bool isConfigurable() const { return true; }

	inline virtual QString name() const { return "Deezer-Plugin"; }

	inline virtual RemoteMediaPlayer * player() const { return _webPlayer;	}

	inline virtual QWidget* providesView() { return NULL; }

	inline virtual QString version() const { return "0.2"; }

	virtual void setSearchDialog(AbstractSearchDialog *w);

	virtual void sync(const QString &token) const;

protected:
	/** Redefined to open Deezer's home page. */
	bool eventFilter(QObject *obj, QEvent *event);

private:
	QString extract(QXmlStreamReader &xml, const QString &criterion);
	void extractAlbum(QNetworkReply *reply, QXmlStreamReader &xml);
	void extractAlbumListFromArtist(QNetworkReply *reply, QXmlStreamReader &xml);
	void extractSynchronizedArtists(QXmlStreamReader &xml);
	void extractTrackListFromAlbum(QNetworkReply *reply, const QString &albumID, QXmlStreamReader &xml);
	void searchRequestFinished(QXmlStreamReader &xml);
	void updateCacheDatabase(const std::list<RemoteTrack> &tracks);

private slots:
	void artistWasDoubleClicked(const QModelIndex &index);
	void albumWasDoubleClicked(const QModelIndex &index);
	void dispatchReply(QNetworkReply *reply);
	void login();
	void saveCredentials(bool enabled);
	void search(const QString &expr);
	void trackWasDoubleClicked(const QModelIndex &index);

signals:
	void searchComplete(AbstractSearchDialog::Request, QList<QStandardItem*> results);
	void aboutToProcessRemoteTracks(const std::list<RemoteTrack> &tracks);
};

#endif // DEEZERPLUGIN_H
