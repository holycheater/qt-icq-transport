TARGET = qt-icq-transport
DESTDIR = ..

include(../common.pri)

QT -= gui

MOC_DIR = .moc/
OBJECTS_DIR = .obj/
QMAKE_DISTCLEAN += $$PWD/debug/ $$PWD/release/ $$PWD/.moc/ $$PWD/.obj/
QMAKE_DEL_FILE = rm -rf

HEADERS += JabberConnection.h
SOURCES += JabberConnection.cpp \
    main.cpp

