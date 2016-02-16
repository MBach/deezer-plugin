QT      += multimedia sql webenginecore webenginewidgets widgets

TARGET   = $$qtLibraryTarget(deezer-plugin)
TEMPLATE = lib

DEFINES += MIAM_PLUGIN

CONFIG  += c++11

win32 {
    MiamPlayerBuildDirectory = C:\dev\Miam-Player-build\src\Player
    CONFIG(debug, debug|release) {
	target.path = $$MiamPlayerBuildDirectory\debug\plugins
	LIBS += -Ldebug -lCore
    }

    CONFIG(release, debug|release) {
	target.path = $$MiamPlayerBuildDirectory\release\plugins
	LIBS += -Lrelease -lCore
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
    model/albumdao.h \
    model/artistdao.h \
    model/genericdao.h \
    model/playlistdao.h \
    model/sqldatabase.h \
    model/trackdao.h \
    model/yeardao.h \
    abstractsearchdialog.h \
    deezerplugin.h \
    deezerwebplayer.h \
    filehelper.h \
    imediaplayer.h \
    miamcore_global.h \
    networkaccessmanager.h \
    settings.h \
    webview.h

SOURCES += deezerplugin.cpp \
    deezerwebplayer.cpp \
    networkaccessmanager.cpp \
    webview.cpp

RESOURCES += resources.qrc

FORMS += config.ui

TRANSLATIONS += translations/Deezer-Plugin_en.ts \
    translations/Deezer-Plugin_fr.ts
