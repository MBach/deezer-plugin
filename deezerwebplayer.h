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

	qint64 _pos;
	qint64 _time;

public:
	explicit DeezerWebPlayer(DeezerPlugin *parent);

	inline virtual QString host() const override { return "www.deezer.com"; }

	/** Current media length in ms. */
	virtual int length() const override;

	/** The position in the current media being played. Percent-based. */
	virtual float position() const override;

	virtual void setMute(bool b) override;

	inline virtual void setPosition(qint64 pos) override { _pos = pos; }

	/** Sets the time in ms in the current media being played (<= length()). */
	inline virtual void setTime(qint64 t) override { _time = t; }

	/** The time in ms in the current media being played (<= length()). */
	inline qint64 time() const override { return _time; }

	WebView * webView() const { return _webView; }

	/** The current volume of this remote player. */
	virtual int volume() const override;

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
