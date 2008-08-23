TARGET = qt-icq-transport
DESTDIR = ..

include(../common.pri)

QT -= gui

HEADERS += JabberConnection.h
SOURCES += JabberConnection.cpp \
    main.cpp

