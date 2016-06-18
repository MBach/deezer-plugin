QT      += multimedia sql widgets

TARGET   = $$qtLibraryTarget(deezer-plugin)
TEMPLATE = lib

INCLUDEPATH += $$PWD
INCLUDEPATH += $$sdk
DEPENDPATH += $$sdk

DEFINES += MIAM_PLUGIN

CONFIG  += dll c++11

win32 {
    MiamPlayerBuildDirectory = C:\dev\Miam-Player-build\src\Player
    CONFIG(debug, debug|release) {
	target.path = $$MiamPlayerBuildDirectory\debug\plugins
	LIBS += -Ldebug -lCore -L$$PWD/lib/ -llibdeezer.x64.dll
    }

    CONFIG(release, debug|release) {
	target.path = $$MiamPlayerBuildDirectory\release\plugins
	LIBS += -Lrelease -lCore -L$$PWD/lib/ -llibdeezer.x64.dll
    }
}

unix {
    MiamPlayerBuildDirectory = /usr/bin
    LIBS += -L$$MiamPlayerBuildDirectory/MiamCore/ -lmiam-core
    QMAKE_CXXFLAGS += -std=c++11
    target.path = $$MiamPlayerBuildDirectory/plugins
}

INSTALLS += target

HEADERS += interfaces/basicplugin.h \
    interfaces/remotemediaplayerplugin.h \
    model/genericdao.h \
    model/playlistdao.h \
    model/sqldatabase.h \
    model/trackdao.h \
    abstractsearchdialog.h \
    deezerplugin.h \
    filehelper.h \
    imediaplayer.h \
    miamcore_global.h \
    networkaccessmanager.h \
    settings.h \
    deezerplayer.h

SOURCES += deezerplugin.cpp \
    networkaccessmanager.cpp \
    deezerplayer.cpp

RESOURCES += resources.qrc

FORMS += config.ui

TRANSLATIONS += translations/Deezer-Plugin_en.ts \
    translations/Deezer-Plugin_fr.ts
