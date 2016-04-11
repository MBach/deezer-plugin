#include "deezerwebplayer.h"
#include "deezerplugin.h"
#include "networkaccessmanager.h"
#include "settings.h"
//#include <QWebFrame>

#include <QtDebug>

DeezerWebPlayer::DeezerWebPlayer(DeezerPlugin *parent)
	: IMediaPlayer(parent)
	//, _webView(new WebView(parent))
	, _webView(nullptr)
	, _deezerPlugin(parent)
	, _stopButtonWasTriggered(false)
	, _pos(0)
	, _time(0)
{
	/// FIXME: how to play sound with invisible webView?
	/*_webView->load(QUrl("http://www.miam-player.org/deezer-micro/index.html"));
	_webView->show();
	Settings *settings = Settings::instance();

	//connect(_webView, &WebView::aboutToSyncWithToken, parent, &DeezerPlugin::sync);
	connect(_webView, &WebView::aboutToSyncWithToken, this, [=](const QString &token) {
		qDebug() << Q_FUNC_INFO << token;
		parent->sync(token);
		_webView->hide();
	});

	// Init callback actions
	connect(_webView->page(), &QWebEnginePage::loadFinished, this, [=](bool ok) {
		qDebug() << "DeezerWebPlayer loadFinished. ok?" << ok;

		_webView->page()->runJavaScript("DZ.ready(function(sdk_options){console.log('dz is ready', sdk_options);}))");

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
}

/** Current media length in ms. */
qint64 DeezerWebPlayer::duration() const
{
	qint64 len = 0;
	_webView->page()->runJavaScript("DZ.player.getCurrentTrack().duration * 1000; ", [&len](QVariant v) {
		qDebug() << Q_FUNC_INFO << v;
		if (v.isValid()) {
			len = v.toLongLong();
		}
	});
	return len;
}

/** The position in the current media being played. Percent-based. */
qreal DeezerWebPlayer::position() const
{
	if (_time > 0) {
		return _pos / (qreal) _time;
	} else {
		return 0;
	}
}

void DeezerWebPlayer::setMute(bool b)
{
	if (b) {
		_webView->page()->runJavaScript("DZ.player.setMute(0);");
	} else {
		_webView->page()->runJavaScript("DZ.player.setMute(1);");
	}
}

qreal DeezerWebPlayer::volume() const
{
	qreal vol = 0.75;
	_webView->page()->runJavaScript("DZ.player.getVolume() / 100; ", [&vol](QVariant v) {
		if (v.isValid()) {
			vol = v.toReal();
		}
	});
	return vol;
}

void DeezerWebPlayer::pause()
{
	_webView->page()->runJavaScript("DZ.player.pause();");
}

void DeezerWebPlayer::play(const QUrl &track)
{
	QString url = track.toString();
	int slash = url.lastIndexOf("/");
	QString id = url.mid(slash + 1);
	QString playTrack = "DZ.player.playTracks([" + id + "]);";
	qDebug() << Q_FUNC_INFO << track << playTrack;
	_webView->page()->runJavaScript(playTrack);
}

void DeezerWebPlayer::resume()
{
	qDebug() << Q_FUNC_INFO;
	_webView->page()->runJavaScript("DZ.player.play();");
}

void DeezerWebPlayer::seek(float pos)
{
	qDebug() << Q_FUNC_INFO << pos;
	_webView->page()->runJavaScript("DZ.player.pause(); DZ.player.seek(" + QString::number(pos * 100) + ");");
}

void DeezerWebPlayer::setVolume(qreal volume)
{
	_webView->page()->runJavaScript("DZ.player.setVolume(Math.round(" + QString::number(volume) + "* 100));");
}

void DeezerWebPlayer::stop()
{
	_pos = 0;
	_time = 0;
	/// FIXME?
	_webView->page()->runJavaScript("DZ.player.isPlaying();", [this](QVariant v) {
		if (v.toBool()) {
			_stopButtonWasTriggered = true;
			_webView->page()->runJavaScript("DZ.player.pause();");
		} else {
			emit stopped();
		}
	});
}

void DeezerWebPlayer::playerHasPaused()
{
	if (_stopButtonWasTriggered) {
		_stopButtonWasTriggered = false;
		emit stopped();
	} else {
		emit paused();
	}
}
