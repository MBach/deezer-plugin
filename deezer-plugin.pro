QT      += multimedia sql webkit webkitwidgets widgets

TARGET   = $$qtLibraryTarget(deezer-plugin)
TEMPLATE = lib


DEFINES += MIAM_PLUGIN

CONFIG  += c++11
# TODO: how to minimize hardcoded paths?
win32 {
    MiamPlayerBuildDirectory = C:\dev\Miam-Player-build-x64\MiamPlayer
    CONFIG(debug, debug|release) {
        target.path = $$MiamPlayerBuildDirectory\debug\plugins
        LIBS += -Ldebug -lMiamCore
    }

    CONFIG(release, debug|release) {
        target.path = $$MiamPlayerBuildDirectory\release\plugins
        LIBS += -Lrelease -lMiamCore
    }
}
unix {
    MiamPlayerBuildDirectory = /home/mbach/Miam-Player-release
    target.path = $$MiamPlayerBuildDirectory/MiamPlayer/plugins
    LIBS += -L$$MiamPlayerBuildDirectory/MiamCore -lmiam-core
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
    autosaver.h \
    cookiejar.h \
    deezerplugin.h \
    deezerwebplayer.h \
    filehelper.h \
    miamcore_global.h \
    networkaccessmanager.h \
    remotemediaplayer.h \
    settings.h \
    webview.h

SOURCES += autosaver.cpp \
    cookiejar.cpp \
    deezerplugin.cpp \
    deezerwebplayer.cpp \
    networkaccessmanager.cpp \
    webview.cpp

RESOURCES += resources.qrc

FORMS += config.ui

TRANSLATIONS += translations/Deezer-Plugin_en.ts \
    translations/Deezer-Plugin_fr.ts
