
include(types/types.pri)
include(managers/managers.pri)

INCLUDEPATH += $$PWD

HEADERS += \
	$$PWD/icqSession.h \
	$$PWD/icqSocket.h
SOURCES += \
	$$PWD/icqSession.cpp \
	$$PWD/icqSocket.cpp

