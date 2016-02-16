QT      += multimedia sql webenginecore webenginewidgets widgets

TARGET   = $$qtLibraryTarget(deezer-plugin)
TEMPLATE = lib

DEFINES += MIAM_PLUGIN

win32 {
    MiamPlayerBuildDirectory = C:\dev\Miam-Player-build\src\Player
    CONFIG  += c++11
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
    MiamPlayerBuildDirectory = /home/mbach/Miam-Player/build-MiamProject-Desktop_Qt_5_5_0_GCC_64bit-Release
    LIBS += -L$$MiamPlayerBuildDirectory/MiamCore/ -lmiam-core
    CONFIG += c++11
    QMAKE_CXXFLAGS += -std=c++11
    target.path = $$MiamPlayerBuildDirectory/MiamPlayer/plugins
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
