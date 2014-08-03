#ifndef DEEZERPLUGIN_H
#define DEEZERPLUGIN_H

#include "miamcore_global.h"
#include "mediaplayerplugininterface.h"
#include "mediaplayer.h"

#include <QObject>
#include <QNetworkAccessManager>

#include "ui_config.h"

class QWebView;

/**
 * \brief       Deezer plugin
 * \author      Matthieu Bachelier
 * \version     0.1
 * \copyright   GNU General Public License v3
 */
class DeezerPlugin : public QObject, public MediaPlayerPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID MediaPlayerPluginInterface_iid)
	Q_INTERFACES(MediaPlayerPluginInterface)

private:
	Ui::DeezerPluginConfigPage _config;

	QWeakPointer<MediaPlayer> _mediaPlayer;
	
	QNetworkAccessManager *_networkAccessManager;
	QList<QWebView*> _pages;
	
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
	
private slots:
	void saveCredentials(bool enabled);
	
	void login();
};

#endif // DEEZERPLUGIN_H
