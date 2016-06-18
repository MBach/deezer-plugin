#include "deezerplugin.h"
#include "networkaccessmanager.h"
#include "settings.h"

#include <QStandardPaths>

#include <QtDebug>

int DeezezPlayer::_activationCount = 0;
int DeezezPlayer::_nbTrackToPlay = 1;
int DeezezPlayer::_nbTrackPlayed = 0;
dz_player_handle DeezezPlayer::_dzplayer = NULL;

DeezezPlayer::DeezezPlayer(DeezerPlugin *parent)
	: IMediaPlayer(parent)
	, _deezerPlugin(parent)
	, _stopButtonWasTriggered(false)
	, _pos(0)
	, _time(0)
{
	this->initSDK();
}

/** Current media length in ms. */
qint64 DeezezPlayer::duration() const
{
	qint64 len = 0;
	/*_webView->page()->runJavaScript("DZ.player.getCurrentTrack().duration * 1000; ", [&len](QVariant v) {
		qDebug() << Q_FUNC_INFO << v;
		if (v.isValid()) {
			len = v.toLongLong();
		}
	});*/
	return len;
}

/** The position in the current media being played. Percent-based. */
qreal DeezezPlayer::position() const
{
	if (_time > 0) {
		return _pos / (qreal) _time;
	} else {
		return 0;
	}
}

void DeezezPlayer::setMute(bool b)
{
	/*if (b) {
		_webView->page()->runJavaScript("DZ.player.setMute(0);");
	} else {
		_webView->page()->runJavaScript("DZ.player.setMute(1);");
	}*/
}

qreal DeezezPlayer::volume() const
{
	qreal vol = 0.75;
	/*_webView->page()->runJavaScript("DZ.player.getVolume() / 100; ", [&vol](QVariant v) {
		if (v.isValid()) {
			vol = v.toReal();
		}
	});*/
	return vol;
}

void DeezezPlayer::initSDK()
{
	dz_error_t dzerr = DZ_ERROR_NO_ERROR;

	qDebug() << "<-- Deezer native SDK Version: " << dz_connect_get_build_id();

	memset(&_config, 0, sizeof(struct dz_connect_configuration));

	_config.app_id            = "141475";
	_config.product_id        = "MiamPlayer";
	_config.product_build_id  = "0.8.1";
	QString cacheLocation = QStandardPaths::standardLocations(QStandardPaths::CacheLocation).first();
	_config.user_profile_path = cacheLocation.toStdString().data();
	_config.connect_event_cb  = appConnectOneventCb;

	qDebug() << "--> Application ID:" << _config.app_id;
	qDebug() << "--> Product ID:", _config.product_id;
	qDebug() << "--> Product BUILD ID:" << _config.product_build_id;
	qDebug() << "--> User Profile Path:" << _config.user_profile_path;

	_dzconnect = dz_connect_new(&_config);
	if (_dzconnect == NULL) {
		qDebug() << "dzconnect null";
		return;
	}

	qDebug() << "Device ID: " << dz_connect_get_device_id(_dzconnect);

	dzerr = dz_connect_debug_log_disable(_dzconnect);
	if (dzerr != DZ_ERROR_NO_ERROR) {
		qDebug() << "dz_connect_debug_log_disable error";
		return;
	}

	dzerr = dz_connect_activate(_dzconnect, NULL);
	if (dzerr != DZ_ERROR_NO_ERROR) {
		qDebug() << "dz_connect_activate error";
		return;
	}
	_activationCount++;

	/* Calling dz_connect_cache_path_set()
	 * is mandatory in order to have the attended behavior */
	dz_connect_cache_path_set(_dzconnect, NULL, NULL, cacheLocation.toStdString().data());

	_dzplayer = dz_player_new(_dzconnect);
	if (_dzplayer == NULL) {
		qDebug() << "dzplayer null";
		return;
	}

	dzerr = dz_player_activate(_dzplayer, NULL);
	if (dzerr != DZ_ERROR_NO_ERROR) {
		qDebug() << "dz_player_activate error";
		return;
	}
	_activationCount++;

	dzerr = dz_player_set_event_cb(_dzplayer, appPlayerOneventCb);
	if (dzerr != DZ_ERROR_NO_ERROR) {
		qDebug() << "dz_player_set_event_cb error";
		return;
	}

	dzerr = dz_connect_set_access_token(_dzconnect,NULL, NULL, USER_ACCESS_TOKEN);
	if (dzerr != DZ_ERROR_NO_ERROR) {
		qDebug() << "dz_connect_set_access_token error";
		return;
	}

	/* Calling dz_connect_offline_mode(FALSE) is mandatory to force the login */
	dzerr = dz_connect_offline_mode(_dzconnect, NULL, NULL, false);
	if (dzerr != DZ_ERROR_NO_ERROR) {
		qDebug() << "dz_connect_offline_mode error";
		return;
	}

	while (1) {
		//qDebug() << "- - - - wait until end of actions (c=%d)\n",activation_count);
		QThread::sleep(1);
		// Exited normally
		if (_activationCount == 0) {
			break;
		}
	}

	if (_dzconnect) {
		dz_object_release((dz_object_handle)_dzconnect);
		_dzconnect = NULL;
	}
	if (_dzplayer) {
		dz_object_release((dz_object_handle)_dzplayer);
		_dzplayer = NULL;
	}

	qDebug() << "-- shutdowned --";
}

void DeezezPlayer::appConnectOneventCb(dz_connect_handle handle, dz_connect_event_handle event, void *delegate)
{
	dz_connect_event_t type = dz_connect_event_get_type(event);
	switch (type) {
	case DZ_CONNECT_EVENT_USER_OFFLINE_AVAILABLE:
		qDebug() << "++++ CONNECT_EVENT ++++ USER_OFFLINE_AVAILABLE";
		break;

	case DZ_CONNECT_EVENT_USER_ACCESS_TOKEN_OK:
	{
		const char* szAccessToken;
		szAccessToken = dz_connect_event_get_access_token(event);
		qDebug() << "++++ CONNECT_EVENT ++++ USER_ACCESS_TOKEN_OK Access_token:" << szAccessToken;
	}
		break;

	case DZ_CONNECT_EVENT_USER_ACCESS_TOKEN_FAILED:
		qDebug() << "++++ CONNECT_EVENT ++++ USER_ACCESS_TOKEN_FAILED";
		break;

	case DZ_CONNECT_EVENT_USER_LOGIN_OK:
		qDebug() << "++++ CONNECT_EVENT ++++ USER_LOGIN_OK";
		appLaunchPlay();
		break;

	case DZ_CONNECT_EVENT_USER_NEW_OPTIONS:
		qDebug() << "++++ CONNECT_EVENT ++++ USER_NEW_OPTIONS";
		break;

	case DZ_CONNECT_EVENT_USER_LOGIN_FAIL_NETWORK_ERROR:
		qDebug() << "++++ CONNECT_EVENT ++++ USER_LOGIN_FAIL_NETWORK_ERROR";
		break;

	case DZ_CONNECT_EVENT_USER_LOGIN_FAIL_BAD_CREDENTIALS:
		qDebug() << "++++ CONNECT_EVENT ++++ USER_LOGIN_FAIL_BAD_CREDENTIALS";
		break;

	case DZ_CONNECT_EVENT_USER_LOGIN_FAIL_USER_INFO:
		qDebug() << "++++ CONNECT_EVENT ++++ USER_LOGIN_FAIL_USER_INFO";
		break;

	case DZ_CONNECT_EVENT_USER_LOGIN_FAIL_OFFLINE_MODE:
		qDebug() << "++++ CONNECT_EVENT ++++ USER_LOGIN_FAIL_OFFLINE_MODE";
		break;

	case DZ_CONNECT_EVENT_ADVERTISEMENT_START:
		qDebug() << "++++ CONNECT_EVENT ++++ ADVERTISEMENT_START";
		break;

	case DZ_CONNECT_EVENT_ADVERTISEMENT_STOP:
		qDebug() << "++++ CONNECT_EVENT ++++ ADVERTISEMENT_STOP";
		break;

	case DZ_CONNECT_EVENT_UNKNOWN:
	default:
		qDebug() << "++++ CONNECT_EVENT ++++ UNKNOWN or default (type = " << type << ")";
		break;
	}
}

void DeezezPlayer::appPlayerOneventCb(dz_player_handle handle, dz_player_event_handle event, void *supervisor)
{
	dz_streaming_mode_t  streaming_mode;
	dz_index_in_playlist idx;

	dz_player_event_t type = dz_player_event_get_type(event);

	if (!dz_player_event_get_playlist_context(event, &streaming_mode, &idx)) {
		streaming_mode = DZ_STREAMING_MODE_ONDEMAND;
		idx = -1;
	}

	switch (type) {
	case DZ_PLAYER_EVENT_LIMITATION_FORCED_PAUSE:
		qDebug() << "==== PLAYER_EVENT ==== LIMITATION_FORCED_PAUSE for idx:" << idx;
		break;

	case DZ_PLAYER_EVENT_PLAYLIST_TRACK_NO_RIGHT:
		qDebug() << "==== PLAYER_EVENT ==== PLAYLIST_TRACK_NO_RIGHT for idx:" << idx;
		break;

	case DZ_PLAYER_EVENT_PLAYLIST_NEED_NATURAL_NEXT:
		qDebug() << "==== PLAYER_EVENT ==== PLAYLIST_NEED_NATURAL_NEXT for idx:" << idx;
		appLaunchPlay();
		break;

	case DZ_PLAYER_EVENT_PLAYLIST_TRACK_NOT_AVAILABLE_OFFLINE:
		qDebug() << "==== PLAYER_EVENT ==== PLAYLIST_TRACK_NOT_AVAILABLE_OFFLINE for idx:" << idx;
		break;

	case DZ_PLAYER_EVENT_PLAYLIST_TRACK_RIGHTS_AFTER_AUDIOADS:
		qDebug() << "==== PLAYER_EVENT ==== PLAYLIST_TRACK_RIGHTS_AFTER_AUDIOADS for idx:" << idx;
		dz_player_play_audioads(_dzplayer, NULL, NULL);
		break;

	case DZ_PLAYER_EVENT_PLAYLIST_SKIP_NO_RIGHT:
		qDebug() << "==== PLAYER_EVENT ==== PLAYLIST_SKIP_NO_RIGHT for idx:" << idx;
		break;

	case DZ_PLAYER_EVENT_PLAYLIST_TRACK_SELECTED:
		{
			bool is_preview;
			bool can_pause_unpause;
			bool can_seek;
			int  nb_skip_allowed;
			const char *selected_dzapiinfo;
			const char *next_dzapiinfo;

			is_preview = dz_player_event_track_selected_is_preview(event);

			dz_player_event_track_selected_rights(event, &can_pause_unpause, &can_seek, &nb_skip_allowed);

			selected_dzapiinfo = dz_player_event_track_selected_dzapiinfo(event);
			next_dzapiinfo = dz_player_event_track_selected_next_track_dzapiinfo(event);

			qDebug() << "==== PLAYER_EVENT ==== PLAYLIST_TRACK_SELECTED for idx: " << idx << "- is_preview:" << is_preview;
			qDebug() << "can_pause_unpause: " << can_pause_unpause << "can_seek: " << can_seek << "nb_skip_allowed:" << nb_skip_allowed;
			if (selected_dzapiinfo)
			qDebug() << "now:" << selected_dzapiinfo;
			if (next_dzapiinfo)
			qDebug() << "next:" << next_dzapiinfo;
		 }
		_nbTrackPlayed++;
		break;

	case DZ_PLAYER_EVENT_MEDIASTREAM_DATA_READY:
		qDebug() << "==== PLAYER_EVENT ==== MEDIASTREAM_DATA_READY for idx:" << idx;
		break;

	case DZ_PLAYER_EVENT_MEDIASTREAM_DATA_READY_AFTER_SEEK:
		qDebug() << "==== PLAYER_EVENT ==== MEDIASTREAM_DATA_READY_AFTER_SEEK for idx:" << idx;
		break;

	case DZ_PLAYER_EVENT_RENDER_TRACK_START_FAILURE:
		qDebug() << "==== PLAYER_EVENT ==== RENDER_TRACK_START_FAILURE for idx:" << idx;
		break;

	case DZ_PLAYER_EVENT_RENDER_TRACK_START:
		qDebug() << "==== PLAYER_EVENT ==== RENDER_TRACK_START for idx:" << idx;
		break;

	case DZ_PLAYER_EVENT_RENDER_TRACK_END:
		qDebug() << "==== PLAYER_EVENT ==== RENDER_TRACK_END for idx:" << idx;
		qDebug() << "nb_track_to_play:" << _nbTrackToPlay << "nb_track_played :" << _nbTrackPlayed;
		if (_nbTrackToPlay != -1 &&  // unlimited
			_nbTrackToPlay == _nbTrackPlayed) {
			appShutdown();
		} else {
			appLaunchPlay();
		}
		break;

	case DZ_PLAYER_EVENT_RENDER_TRACK_PAUSED:
		qDebug() << "==== PLAYER_EVENT ==== RENDER_TRACK_PAUSED for idx:" << idx;
		break;

	case DZ_PLAYER_EVENT_RENDER_TRACK_UNDERFLOW:
		qDebug() << "==== PLAYER_EVENT ==== RENDER_TRACK_UNDERFLOW for idx:" << idx;
		break;

	case DZ_PLAYER_EVENT_RENDER_TRACK_RESUMED:
		qDebug() << "==== PLAYER_EVENT ==== RENDER_TRACK_RESUMED for idx:" << idx;
		break;

	case DZ_PLAYER_EVENT_RENDER_TRACK_SEEKING:
		qDebug() << "==== PLAYER_EVENT ==== RENDER_TRACK_SEEKING for idx:" << idx;
		break;

	case DZ_PLAYER_EVENT_RENDER_TRACK_REMOVED:
		qDebug() << "==== PLAYER_EVENT ==== RENDER_TRACK_REMOVED for idx:" << idx;
		break;

	case DZ_PLAYER_EVENT_UNKNOWN:
	default:
		qDebug() << "==== PLAYER_EVENT ==== UNKNOWN or default (type = " << type << ")";
		break;
	}
}

void DeezezPlayer::appLaunchPlay() {
	static char sz_track_url[256] = "dzmedia:///track/85509044";
	qDebug() << "LOAD & PLAY => " << sz_track_url;
	dz_player_load(_dzplayer, NULL, NULL, sz_track_url);
	dz_player_play(_dzplayer, NULL, NULL, DZ_PLAYER_PLAY_CMD_START_TRACKLIST, DZ_TRACKLIST_AUTOPLAY_MANUAL, 0);
}

void DeezezPlayer::appShutdown() {
	qDebug() << "SHUTDOWN (1/2) - dzplayer = " << _dzplayer;
	if (_dzplayer) {
		dz_player_deactivate(_dzplayer, dzPlayerOnDeactivate, NULL);
	}

	//qDebug() << "SHUTDOWN (2/2) - dzconnect = " << _dzconnect;
	/*if (_dzconnect != NULL) {
		dz_connect_deactivate(_dzconnect, dz_connect_on_deactivate, NULL);
	}*/
}

void DeezezPlayer::dzPlayerOnDeactivate(void* delegate, void* operation_userdata, dz_error_t status, dz_object_handle result)
{
	_activationCount--;
	qDebug() << "PLAYER deactivated - c = " << _activationCount << " with status = " << status;
}

void DeezezPlayer::pause()
{
	//_webView->page()->runJavaScript("DZ.player.pause();");
}

void DeezezPlayer::play(const QUrl &track)
{
	QString url = track.toString();
	int slash = url.lastIndexOf("/");
	QString id = url.mid(slash + 1);
	QString playTrack = "DZ.player.playTracks([" + id + "]);";
	qDebug() << Q_FUNC_INFO << track << playTrack;
	//_webView->page()->runJavaScript(playTrack);
}

void DeezezPlayer::resume()
{
	qDebug() << Q_FUNC_INFO;
	//_webView->page()->runJavaScript("DZ.player.play();");
}

void DeezezPlayer::seek(float pos)
{
	qDebug() << Q_FUNC_INFO << pos;
	//_webView->page()->runJavaScript("DZ.player.pause(); DZ.player.seek(" + QString::number(pos * 100) + ");");
}

void DeezezPlayer::setVolume(qreal volume)
{
	//_webView->page()->runJavaScript("DZ.player.setVolume(Math.round(" + QString::number(volume) + "* 100));");
}

void DeezezPlayer::stop()
{
	_pos = 0;
	_time = 0;
	/// FIXME?
	/*_webView->page()->runJavaScript("DZ.player.isPlaying();", [this](QVariant v) {
		if (v.toBool()) {
			_stopButtonWasTriggered = true;
			_webView->page()->runJavaScript("DZ.player.pause();");
		} else {
			emit stopped();
		}
	});*/
}

void DeezezPlayer::playerHasPaused()
{
	if (_stopButtonWasTriggered) {
		_stopButtonWasTriggered = false;
		emit stopped();
	} else {
		emit paused();
	}
}
