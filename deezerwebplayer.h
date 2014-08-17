#ifndef DEEZERWEBPLAYER_H
#define DEEZERWEBPLAYER_H

#include <QMediaPlayer>
#include <QObject>
#include <QUrl>
#include "webview.h"
#include "remotemediaplayer.h"

class DeezerPlugin;

class DeezerWebPlayer : public RemoteMediaPlayer
{
	Q_OBJECT
private:
	WebView *_webView;
	DeezerPlugin *_deezerPlugin;

	// Javascript SDK has no "stop" behaviour
	bool _stopButtonWasTriggered;

public:
	explicit DeezerWebPlayer(DeezerPlugin *parent);

	WebView * webView() const { return _webView; }

public slots:
	void pause();
	void play(const QUrl &track);
	void resume(const QUrl &);
	void seek(float pos);
	void setVolume(int volume);
	void stop();

	void playerHasPaused();
	void playerHasStarted(int duration);

signals:
	void positionChanged(double pos, double duration);
	void trackHasEnded();
};

#endif // DEEZERWEBPLAYER_H
