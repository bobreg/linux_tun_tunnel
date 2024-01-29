#-------------------------------------------------
#
# Project created by QtCreator 2023-03-22T17:35:48
#
#-------------------------------------------------

QT       += core network
QT -= gui

greaterThan(QT_MAJOR_VERSION, 4): QT -= widgets

OBJECTS_DIR = trash/.obj
MOC_DIR = trash/.moc
DESTDIR = build

TARGET = tun_tunnel
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
    application.cpp \
    logger.cpp \
    Network.cpp

HEADERS += \
    application.h \
    globals.h \
    logger.h \
    Network.h

DISTFILES += \
    build/settings.ini
