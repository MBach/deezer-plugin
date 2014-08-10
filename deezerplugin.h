#ifndef DEEZERPLUGIN_H
#define DEEZERPLUGIN_H

#include "miamcore_global.h"
#include "mediaplayer.h"
#include "searchmediaplayerplugin.h"

#include <QNetworkReply>

#include "ui_config.h"

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
	QListWidget *_artists;
	QListWidget *_albums;
	QListWidget *_tracks;
	QCheckBox *_checkBox;

protected:
	bool eventFilter(QObject *obj, QEvent *event);

public:
	explicit DeezerPlugin();

	virtual ~DeezerPlugin();

	virtual QWidget* configPage();

	virtual void init();

	inline virtual bool isConfigurable() const { return true; }

	inline virtual QString name() const { return "Deezer-Plugin"; }

	inline virtual QWidget* providesView() { return NULL; }

	virtual void setMediaPlayer(QWeakPointer<MediaPlayer> mediaPlayer);

	inline virtual QString version() const { return "0.1"; }

	virtual void setSearchDialog(AbstractSearchDialog *w);
	virtual void dispatchResults(Request request, QListWidget *list);

private slots:
	void login();
	void replyFinished(QNetworkReply *);
	void saveCredentials(bool enabled);
	void search(const QString &expr);

signals:
	void searchComplete(SearchMediaPlayerPlugin::Request, QList<QListWidgetItem*> results);
};

#endif // DEEZERPLUGIN_H
