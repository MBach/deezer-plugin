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

	inline virtual QString host() const { return "www.deezer.com"; }

	WebView * webView() const { return _webView; }

public slots:
	virtual void pause();
	virtual void play(const QUrl &track);
	virtual void resume();
	virtual void seek(float pos);
	virtual void setVolume(int volume);
	virtual void stop();

	void playerHasPaused();
};

#endif // DEEZERWEBPLAYER_H
