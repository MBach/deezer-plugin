#include "deezerplugin.h"
#include "networkaccessmanager.h"
#include "settings.h"

#include "sdk/deezer-connect.h"

#include <QtDebug>

DeezezPlayer::DeezezPlayer(DeezerPlugin *parent)
	: IMediaPlayer(parent)
	, _deezerPlugin(parent)
	, _stopButtonWasTriggered(false)
	, _pos(0)
	, _time(0)
{
	/// FIXME: how to play sound with invisible webView?
	//_webView->load(QUrl("http://www.miam-player.org/deezer-micro/index.html"));
	//_webView->show();
	Settings *settings = Settings::instance();

	//connect(_webView, &WebView::aboutToSyncWithToken, parent, &DeezerPlugin::sync);
	/*connect(_webView, &WebView::aboutToSyncWithToken, this, [=](const QString &token) {
		qDebug() << Q_FUNC_INFO << token;
		parent->sync(token);
		_webView->hide();
	});*/
	/*connect(_webView, &WebView::loadFinished, this, [=](bool success) {
		qDebug() << Q_FUNC_INFO << "loadFinished ?" << success;
		//parent->sync(token);
		_webView->hide();
	});*/

	// Init callback actions
	/*connect(_webView->page(), &QWebEnginePage::loadFinished, this, [=](bool ok) {
		qDebug() << "DeezerWebPlayer loadFinished. ok?" << ok;

		_webView->page()->runJavaScript("DZ.ready(function(sdk_options){console.log('dz is ready', sdk_options);}))", [=]() {
			qDebug() << "callback DZ.ready()" <<
		});

		QString posChanged = "DZ.Event.subscribe('player_position', function(a){ " \
			"dzWebPlayer.positionChanged(1000 * a[0], 1000 * a[1]); });";
		_webView->page()->runJavaScript(posChanged);

		QString playerStarted = "DZ.Event.subscribe('player_play', function(){ " \
			"var t = DZ.player.getCurrentTrack(); " \
			"dzWebPlayer.started(1000 * t.duration); });";
		_webView->page()->runJavaScript(playerStarted);

		QString playerPaused = "DZ.Event.subscribe('player_paused', function(){ dzWebPlayer.playerHasPaused(); });";
		_webView->page()->runJavaScript(playerPaused);

		QString trackEnded = "DZ.Event.subscribe('track_end', function(){ dzWebPlayer.trackHasEnded(); });";
		_webView->page()->runJavaScript(trackEnded);

		this->setVolume(settings->volume());

		if (_webView->isVisible()) {
			_webView->hide();
		}
	});*/

	dz_connect_onevent_cb connect_event_cb;

	dz_connect_configuration *configuration = new dz_connect_configuration;

	char *app_id = (char *)malloc(7);
	strcpy(app_id, "141475");
	configuration->app_id = app_id;

	char *product_id = (char *)malloc(11);
	strcpy(product_id, "MiamPlayer");
	configuration->product_id = product_id;

	char *product_build_id = (char *)malloc(6);
	strcpy(product_build_id, "0.8.1");
	configuration->product_build_id = product_build_id;

	char *user_profile_path = (char *)malloc(4);
	strcpy(user_profile_path, "C:\\");
	configuration->user_profile_path = user_profile_path;

	configuration->connect_event_cb = connect_event_cb;

	dz_connect_handle handle = dz_connect_new(configuration);
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
