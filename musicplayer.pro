QT += core gui multimedia widgets
QT += core gui multimedia widgets network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
greaterThan(QT_MAJOR_VERSION, 5): QT += multimediawidgets

CONFIG += c++11

SOURCES += \
    floatinglyrics.cpp \
    main.cpp \
    music.cpp \
    playlistanimator.cpp \
    windowdragcontroller.cpp \
    musicplayer.cpp \
    playlistmanager.cpp \
    localmusicmanager.cpp

HEADERS += \
    floatinglyrics.h \
    music.h \
    playlistanimator.h \
    windowdragcontroller.h \
    musicplayer.h \
    playlistmanager.h \
    localmusicmanager.h

FORMS += \
    floatinglyrics.ui \
    music.ui

TRANSLATIONS += \
    musicplayer_yue_CN.ts

CONFIG += lrelease
CONFIG += embed_translations

RESOURCES += \
    res.qrc

RC_FILE = ico.rc
