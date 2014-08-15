#include "deezerwebplayer.h"
#include "deezerplugin.h"
#include "networkaccessmanager.h"
#include "settings.h"
#include <QWebFrame>

#include <QtDebug>


DeezerWebPlayer::DeezerWebPlayer(DeezerPlugin *parent) :
	QObject(parent)
{
	/// XXX: how to play sound with invisible webView?
	_webView = new WebView;
	_webView->load(QUrl("http://mbach.github.io/Miam-Player/deezer-light/index.html"));
	_webView->show();
	_webView->page()->mainFrame()->addToJavaScriptWindowObject("dzWebPlayer", this);

	connect(_webView->page()->mainFrame(), &QWebFrame::javaScriptWindowObjectCleared, this, [=]() {
		qDebug() << "warning, objets are cleared!";
	});

	Settings *settings = Settings::getInstance();

	// Init callback actions
	connect(_webView, &QWebView::loadFinished, this, [=]() {
		QString positionChanged = "DZ.Event.subscribe('player_position', function(arg){ dzWebPlayer.positionChanged(arg[0], arg[1]); });";
		_webView->page()->mainFrame()->evaluateJavaScript(positionChanged);

		// QMediaPlayer::PausedState == 2
		// QString playerPaused = "DZ.Event.subscribe('player_paused', function(arg){ dzWebPlayer.stateChanged(2); });";
		// _webView->page()->mainFrame()->evaluateJavaScript(playerPaused);

		QString playerPaused = "DZ.Event.subscribe('player_paused', function(arg){ dzWebPlayer.paused(); });";
		_webView->page()->mainFrame()->evaluateJavaScript(playerPaused);

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
	QString playTrack = "DZ.player.playTracks([" + id + "], 0, function(response){ dzWebPlayer.log('load?'); });";
	_webView->page()->mainFrame()->evaluateJavaScript(playTrack);
	emit stateChanged(QMediaPlayer::PlayingState);
}

void DeezerWebPlayer::setVolume(int volume)
{
	_webView->page()->mainFrame()->evaluateJavaScript("DZ.player.setVolume(" + QString::number(volume) + ");");
}

void DeezerWebPlayer::paused()
{
	qDebug() << Q_FUNC_INFO;
	emit stateChanged(QMediaPlayer::PausedState);
}

void DeezerWebPlayer::positionChanged(const QVariant &currentPosition, const QVariant &total)
{
	qDebug() << Q_FUNC_INFO << currentPosition << total;
}

void DeezerWebPlayer::log(const QVariant &callBack)
{
	qDebug() << Q_FUNC_INFO << callBack;
}
