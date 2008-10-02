include($$PWD/xmpp-core/xmpp-core.pri)
include($$PWD/xmpp-ext/xmpp-ext.pri)

INCLUDEPATH += $$PWD

HEADERS += \
	$$PWD/ComponentStream.h
SOURCES += \
	$$PWD/ComponentStream.cpp \
	$$PWD/StreamError.cpp
