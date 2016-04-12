#ifndef DEEZERPLAYER_H
#define DEEZERPLAYER_H

#include <QMediaPlayer>
#include <QObject>
#include <QUrl>
#include "imediaplayer.h"

class DeezerPlugin;

class DeezezPlayer : public IMediaPlayer
{
	Q_OBJECT
private:
	DeezerPlugin *_deezerPlugin;

	bool _stopButtonWasTriggered;

	qint64 _pos;
	qint64 _time;

public:
	explicit DeezezPlayer(DeezerPlugin *parent);

	inline virtual QString host() const override { return "www.deezer.com"; }

	/** Current media length in ms. */
	virtual qint64 duration() const override;

	/** The position in the current media being played. Percent-based. */
	virtual qreal position() const override;

	virtual void setMute(bool b) override;

	inline virtual void setPosition(qint64 pos) override { _pos = pos; }

	/** Sets the time in ms in the current media being played (<= length()). */
	inline virtual void setTime(qint64 t) override { _time = t; }

	/** The time in ms in the current media being played (<= length()). */
	inline qint64 time() const override { return _time; }

	/** The current volume of this remote player. */
	virtual qreal volume() const override;

public slots:
	virtual void pause() override;
	virtual void play(const QUrl &track) override;
	virtual void resume() override;
	virtual void seek(float pos) override;
	virtual void setVolume(qreal volume) override;
	virtual void stop() override;

	void playerHasPaused();
};

#endif // DEEZERPLAYER_H
