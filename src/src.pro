TARGET = qt-icq-transport
DESTDIR = ..
include(../common.pri)
QT -= gui
MOC_DIR = .moc
OBJECTS_DIR = .obj
QMAKE_DISTCLEAN += $$PWD/debug \
    $$PWD/release \
    $$PWD/.moc \
    $$PWD/.obj
QMAKE_DEL_FILE = rm \
    -rf
HEADERS += Stanza.h \
    IQ.h \
    ComponentStream.h \
    GatewayTask.h \
    JabberConnection.h
SOURCES += Stanza.cpp \
    IQ.cpp \
    ComponentStream.cpp \
    GatewayTask.cpp \
    JabberConnection.cpp \
    main.cpp
