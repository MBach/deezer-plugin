QT      += widgets multimedia webkitwidgets

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

HEADERS += basicplugininterface.h \
    mediaplayer.h \
    mediaplayerplugininterface.h \
    miamcore_global.h \
    settings.h \
    filehelper.h \
    deezerplugin.h \
    webview.h \
    cookiejar.h \
    autosaver.h

SOURCES += \
    deezerplugin.cpp \
    webview.cpp \
    cookiejar.cpp \
    autosaver.cpp

RESOURCES += \
    resources.qrc

FORMS += \
    config.ui

TRANSLATIONS += translations/Deezer-Plugin_en.ts \
    translations/Deezer-Plugin_fr.ts

OTHER_FILES += \
    player_basic.html
