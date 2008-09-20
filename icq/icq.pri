
include(types/types.pri)
include(managers/managers.pri)

INCLUDEPATH += $$PWD

HEADERS += \
	$$PWD/icqConnection.h \
	$$PWD/icqConnection_p.h
SOURCES += \
	$$PWD/icqConnection.cpp \
	$$PWD/icqConnection_p.cpp

