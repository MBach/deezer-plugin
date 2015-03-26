#include "deezerwebplayer.h"
#include "deezerplugin.h"
#include "networkaccessmanager.h"
#include "settings.h"
#include <QWebFrame>

#include <QtDebug>

DeezerWebPlayer::DeezerWebPlayer(DeezerPlugin *parent) :
	RemoteMediaPlayer(parent), _webView(new WebView),  _deezerPlugin(parent), _stopButtonWasTriggered(false)
  , _pos(0), _time(0)
{
	/// FIXME: how to play sound with invisible webView?
	_webView->load(QUrl("http://www.miam-player.org/deezer-micro/index.html"));
	_webView->show();
	_webView->page()->setView(_webView);
	_webView->page()->mainFrame()->addToJavaScriptWindowObject("dzWebPlayer", this);

	Settings *settings = Settings::instance();

	// Init callback actions
	connect(_webView->page()->mainFrame(), &QWebFrame::loadFinished, this, [=]() {
		qDebug() << "loadFinished";

		QString posChanged = "DZ.Event.subscribe('player_position', function(a){ " \
			"dzWebPlayer.positionChanged(1000 * a[0], 1000 * a[1]); });";
		_webView->page()->mainFrame()->evaluateJavaScript(posChanged);

		QString playerStarted = "DZ.Event.subscribe('player_play', function(){ " \
			"var t = DZ.player.getCurrentTrack(); " \
			"dzWebPlayer.started(1000 * t.duration); });";
		_webView->page()->mainFrame()->evaluateJavaScript(playerStarted);

		QString playerPaused = "DZ.Event.subscribe('player_paused', function(){ dzWebPlayer.playerHasPaused(); });";
		_webView->page()->mainFrame()->evaluateJavaScript(playerPaused);

		QString trackEnded = "DZ.Event.subscribe('track_end', function(){ dzWebPlayer.trackHasEnded(); });";
		_webView->page()->mainFrame()->evaluateJavaScript(trackEnded);

		this->setVolume(settings->volume());

		/// XXX: auto-hiding but must be changed later!
		if (_webView->isVisible()) {
			_webView->close();
		}
	});
}

/** Current media length in ms. */
int DeezerWebPlayer::length() const
{
	QVariant l = _webView->page()->mainFrame()->evaluateJavaScript("DZ.player.getCurrentTrack().duration * 1000; ");
	qDebug() << Q_FUNC_INFO << l;
	if (l.isValid()) {
		return l.toInt();
	} else {
		return 0;
	}
}

/** The position in the current media being played. Percent-based. */
float DeezerWebPlayer::position() const
{
	if (_time > 0) {
		return _pos / (float) _time;
	} else {
		return 0;
	}
}

void DeezerWebPlayer::setMute(bool b)
{
	QVariant v;
	if (b) {
		v = _webView->page()->mainFrame()->evaluateJavaScript("DZ.player.setMute(0);");
	} else {
		v = _webView->page()->mainFrame()->evaluateJavaScript("DZ.player.setMute(1);");
	}
	qDebug() << Q_FUNC_INFO << v;
}

int DeezerWebPlayer::volume() const
{
	QVariant v = _webView->page()->mainFrame()->evaluateJavaScript("DZ.player.getVolume();");
	if (v.isValid()) {
		return v.toInt();
	} else {
		return 75;
	}
}

void DeezerWebPlayer::pause()
{
	_webView->page()->mainFrame()->evaluateJavaScript("DZ.player.pause();");
}

void DeezerWebPlayer::play(const QUrl &track)
{
	qDebug() << Q_FUNC_INFO << track;
	QString url = track.toString();
	int slash = url.lastIndexOf("/");
	QString id = url.mid(slash + 1);
	QString playTrack = "DZ.player.playTracks([" + id + "]);";
	_webView->page()->mainFrame()->evaluateJavaScript(playTrack);
}

void DeezerWebPlayer::resume()
{
	qDebug() << Q_FUNC_INFO;
	_webView->page()->mainFrame()->evaluateJavaScript("DZ.player.play();");
}

void DeezerWebPlayer::seek(float pos)
{
	qDebug() << Q_FUNC_INFO << pos;
	_webView->page()->mainFrame()->evaluateJavaScript("DZ.player.pause(); DZ.player.seek(" + QString::number(pos * 100) + ");");
}

void DeezerWebPlayer::setVolume(int volume)
{
	_webView->page()->mainFrame()->evaluateJavaScript("DZ.player.setVolume(" + QString::number(volume) + ");");
}

void DeezerWebPlayer::stop()
{
	_pos = 0;
	_time = 0;
	QVariant v = _webView->page()->mainFrame()->evaluateJavaScript("DZ.player.isPlaying();");
	if (v.toBool()) {
		_stopButtonWasTriggered = true;
		_webView->page()->mainFrame()->evaluateJavaScript("DZ.player.pause();");
	} else {
		emit stopped();
	}
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
