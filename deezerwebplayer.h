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

	inline virtual QString host() const override { return "www.deezer.com"; }

	virtual float position() const override { return 0; }

	virtual void setTime(int) override { /** TODO */ }

	WebView * webView() const { return _webView; }

public slots:
	virtual void pause() override;
	virtual void play(const QUrl &track) override;
	virtual void resume() override;
	virtual void seek(float pos) override;
	virtual void setVolume(int volume) override;
	virtual void stop() override;

	void playerHasPaused();
};

#endif // DEEZERWEBPLAYER_H
