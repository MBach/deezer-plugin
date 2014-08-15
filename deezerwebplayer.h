#ifndef DEEZERWEBPLAYER_H
#define DEEZERWEBPLAYER_H

#include <QMediaPlayer>
#include <QObject>
#include <QUrl>
#include "webview.h"

class DeezerPlugin;

class DeezerWebPlayer : public QObject
{
	Q_OBJECT
private:
	WebView *_webView;
	DeezerPlugin *_deezerPlugin;

public:
	explicit DeezerWebPlayer(DeezerPlugin *parent);

public slots:
	void pause();
	void play(const QUrl &track);
	void setVolume(int volume);

	void log(const QVariant &callBack);
	void paused();
	void positionChanged(const QVariant &currentPosition, const QVariant &total);

signals:
	void stateChanged(QMediaPlayer::State);
};

#endif // DEEZERWEBPLAYER_H
