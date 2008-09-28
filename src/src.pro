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
HEADERS += GatewayTask.h \
	JabberConnection.h \
	Options.h
SOURCES += GatewayTask.cpp \
	JabberConnection.cpp \
	Options.cpp \
	main.cpp
