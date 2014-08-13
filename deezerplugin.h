#ifndef DEEZERPLUGIN_H
#define DEEZERPLUGIN_H

#include "miamcore_global.h"
#include "mediaplayer.h"
#include "searchmediaplayerplugin.h"
#include "model/remotetrack.h"

#include <QNetworkReply>
#include <QXmlStreamReader>
#include "ui_config.h"
#include "deezerwebplayer.h"

class QWebView;

/**
 * \brief       Deezer plugin
 * \author      Matthieu Bachelier
 * \version     0.1
 * \copyright   GNU General Public License v3
 */
class DeezerPlugin : public QObject, public SearchMediaPlayerPlugin
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID SearchMediaPlayerPlugin_iid)
	Q_INTERFACES(SearchMediaPlayerPlugin)

private:
	Ui::DeezerPluginConfigPage _config;

	QWeakPointer<MediaPlayer> _mediaPlayer;
	AbstractSearchDialog* _searchDialog;
	QList<QWebView*> _pages;
	QListView *_artists;
	QListView *_albums;
	QListView *_tracks;
	QCheckBox *_checkBox;

	DeezerWebPlayer *_webPlayer;

protected:
	bool eventFilter(QObject *obj, QEvent *event);

public:
	explicit DeezerPlugin();

	virtual ~DeezerPlugin();

	virtual QWidget* configPage();

	inline virtual bool isConfigurable() const { return true; }

	inline virtual QString name() const { return "Deezer-Plugin"; }

	inline virtual QWidget* providesView() { return NULL; }

	virtual void setMediaPlayer(QWeakPointer<MediaPlayer> mediaPlayer);

	inline virtual QString version() const { return "0.1"; }

	virtual void setSearchDialog(AbstractSearchDialog *w);

private slots:
	void login();
	void extractAlbum(QXmlStreamReader &xml);
	void searchRequestFinished(QXmlStreamReader &xml);
	void saveCredentials(bool enabled);
	void search(const QString &expr);

	void artistWasDoubleClicked(const QModelIndex &index);
	void albumWasDoubleClicked(const QModelIndex &index);
	void trackWasDoubleClicked(const QModelIndex &index);

signals:
	void searchComplete(AbstractSearchDialog::Request, QList<QStandardItem*> results);
	void aboutToProcessRemoteTracks(const std::list<RemoteTrack> &tracks);
};

#endif // DEEZERPLUGIN_H
