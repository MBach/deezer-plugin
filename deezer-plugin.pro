QT      += multimedia sql webkit webkitwidgets widgets

TARGET   = $$qtLibraryTarget(deezer-plugin)
TEMPLATE = lib

MiamPlayerBuildDirectory = C:\dev\Miam-Player-build-x64\MiamPlayer

DEFINES += MIAM_PLUGIN

CONFIG  += c++11
CONFIG(debug, debug|release) {
    target.path = $$MiamPlayerBuildDirectory\debug\plugins
    LIBS += -Ldebug -lMiamCore
}

CONFIG(release, debug|release) {
    target.path = $$MiamPlayerBuildDirectory\release\plugins
    LIBS += -Lrelease -lMiamCore
}

INSTALLS += target

HEADERS += model/genericdao.h \
    model/playlistdao.h \
    model/trackdao.h \
    model/sqldatabase.h \
    abstractsearchdialog.h \
    autosaver.h \
    basicplugin.h \
    cookiejar.h \
    deezerdatabase.h \
    deezerplugin.h \
    deezerwebplayer.h \
    filehelper.h \
    miamcore_global.h \
    networkaccessmanager.h \
    remotemediaplayer.h \
    remotemediaplayerplugin.h \
    settings.h \
    webview.h

SOURCES += autosaver.cpp \
    cookiejar.cpp \
    deezerdatabase.cpp \
    deezerplugin.cpp \
    deezerwebplayer.cpp \
    networkaccessmanager.cpp \
    webview.cpp

RESOURCES += \
    resources.qrc

FORMS += \
    config.ui

TRANSLATIONS += translations/Deezer-Plugin_en.ts \
    translations/Deezer-Plugin_fr.ts
