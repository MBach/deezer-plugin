#include "deezerwebplayer.h"
#include "deezerplugin.h"
#include "networkaccessmanager.h"
#include "settings.h"
#include <QWebFrame>

#include <QtDebug>

DeezerWebPlayer::DeezerWebPlayer(DeezerPlugin *parent) :
	RemoteMediaPlayer(parent), _webView(new WebView),  _deezerPlugin(parent), _stopButtonWasTriggered(false)
{
	/// FIXME: how to play sound with invisible webView?
	_webView->load(QUrl("http://bachelierm.free.fr/miamplayer/index.html"));
	_webView->show();
	_webView->page()->mainFrame()->addToJavaScriptWindowObject("dzWebPlayer", this);

	connect(_webView->page()->mainFrame(), &QWebFrame::javaScriptWindowObjectCleared, this, [=]() {
		qDebug() << "warning, objets are cleared!";
	});

	Settings *settings = Settings::getInstance();

	// Init callback actions
	connect(_webView->page()->mainFrame(), &QWebFrame::loadFinished, this, [=]() {
		qDebug() << "loadFinished" << _webView->page()->mainFrame()->frameName();

		QString positionChanged = "DZ.Event.subscribe('player_position', function(a){ dzWebPlayer.positionChanged(1000 * a[0], 1000 * a[1]); });";
		_webView->page()->mainFrame()->evaluateJavaScript(positionChanged);

		QString playerStarted = "DZ.Event.subscribe('player_play', function(){ \
				var t = DZ.player.getCurrentTrack(); \
				dzWebPlayer.playerHasStarted(t.duration); });";
		_webView->page()->mainFrame()->evaluateJavaScript(playerStarted);

		QString playerPaused = "DZ.Event.subscribe('player_paused', function(){ dzWebPlayer.playerHasPaused(); });";
		_webView->page()->mainFrame()->evaluateJavaScript(playerPaused);

		QString trackEnded = "DZ.Event.subscribe('track_end', function(){ dzWebPlayer.trackHasEnded(); });";
		_webView->page()->mainFrame()->evaluateJavaScript(trackEnded);

		this->setVolume(settings->volume());
	});
}

void DeezerWebPlayer::pause()
{
	_webView->page()->mainFrame()->evaluateJavaScript("DZ.player.pause();");
}

void DeezerWebPlayer::play(const QUrl &track)
{
	QString url = track.toString();
	int slash = url.lastIndexOf("/");
	QString id = url.mid(slash + 1);
	QString playTrack = "DZ.player.playTracks([" + id + "]);";
	_webView->page()->mainFrame()->evaluateJavaScript(playTrack);
}

void DeezerWebPlayer::resume(const QUrl &)
{
	_webView->page()->mainFrame()->evaluateJavaScript("DZ.player.play();");
}

void DeezerWebPlayer::seek(float pos)
{
	_webView->page()->mainFrame()->evaluateJavaScript("DZ.player.seek(" + QString::number(pos * 100) + ");");
}

void DeezerWebPlayer::setVolume(int volume)
{
	_webView->page()->mainFrame()->evaluateJavaScript("DZ.player.setVolume(" + QString::number(volume) + ");");
}

void DeezerWebPlayer::stop()
{
	QVariant v = _webView->page()->mainFrame()->evaluateJavaScript("DZ.player.isPlaying();");
	if (v.toBool()) {
		_stopButtonWasTriggered = true;
		_webView->page()->mainFrame()->evaluateJavaScript("DZ.player.pause();");
	} else {
		//_deezerPlugin->mediaPlayer().data()->setState(QMediaPlayer::StoppedState);
	}
}

void DeezerWebPlayer::playerHasPaused()
{
	if (_stopButtonWasTriggered) {
		//_deezerPlugin->mediaPlayer().data()->setState(QMediaPlayer::StoppedState);
		_stopButtonWasTriggered = false;
	} else {
		//_deezerPlugin->mediaPlayer().data()->setState(QMediaPlayer::PausedState);
	}
}

void DeezerWebPlayer::playerHasStarted(int duration)
{
	qDebug() << duration;
	//_deezerPlugin->mediaPlayer().data()->setState(QMediaPlayer::PlayingState);
}
