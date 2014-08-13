#include "deezerwebplayer.h"

#include <QQmlComponent>
#include <QQmlEngine>

#include <QtDebug>

DeezerWebPlayer::DeezerWebPlayer(QObject *parent) :
	QObject(parent)
{
	QQmlEngine engine;
	QQmlComponent component(&engine, QUrl("qrc:///dz.qml"));
	QObject *object = component.create();

	QVariant returnedValue;
	QVariant msg = "Hello from C++";
	QMetaObject::invokeMethod(object, "helloDZ", Q_RETURN_ARG(QVariant, returnedValue), Q_ARG(QVariant, msg));
	qDebug() << "QML function returned:" << returnedValue.toString();
	delete object;
	qDebug() << component.errors();
}

void DeezerWebPlayer::play(const QUrl &track)
{
	qDebug() << Q_FUNC_INFO << track;

	/*QScriptValue dz = _engine.evaluate(contents);
	DZ.player.playTracks([id], 0, function(response){
		console.log(LOGNS, "track list", response.tracks);
	});*/


}
