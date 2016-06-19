#ifndef DEEZERPLAYER_H
#define DEEZERPLAYER_H

#include <QMediaPlayer>
#include <QObject>
#include <QUrl>
#include "imediaplayer.h"

#include "sdk/deezer-connect.h"
#include "sdk/deezer-player.h"

// Sample access token corresponding to a free user account, to be replaced by yours.
#define USER_ACCESS_TOKEN "fr49mph7tV4KY3ukISkFHQysRpdCEbzb958dB320pM15OpFsQs"

class DeezerPlugin;

class DeezezPlayer : public IMediaPlayer
{
	Q_OBJECT
private:
	DeezerPlugin *_deezerPlugin;

	bool _stopButtonWasTriggered;

	qint64 _pos;
	qint64 _time;

	dz_connect_configuration _config;
	static dz_connect_handle _dzconnect;
	static int _activationCount;
	static int _nbTrackPlayed;
	static int _nbTrackToPlay;
	static dz_player_handle _dzplayer;

public:
	explicit DeezezPlayer(DeezerPlugin *parent);

	void init();

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

private:

	static void appConnectOnEventCb(dz_connect_handle handle, dz_connect_event_handle event, void* delegate);

	static void appPlayerOnEventCb(dz_player_handle handle, dz_player_event_handle event, void *supervisor);

	static void appLaunchPlay();

	static void appShutdown();

	static void deactivate(void* delegate, void* operation_userdata, dz_error_t status, dz_object_handle result);

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
