#ifndef DEEZERWEBPLAYER_H
#define DEEZERWEBPLAYER_H

#include <QJSEngine>
#include <QObject>
#include <QScriptEngine>
#include <QUrl>

class DeezerWebPlayer : public QObject
{
	Q_OBJECT
private:
	QScriptEngine _engine;

public:
	explicit DeezerWebPlayer(QObject *parent = 0);

public slots:
	void play(const QUrl &track);
};

#endif // DEEZERWEBPLAYER_H
