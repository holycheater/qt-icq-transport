include($$PWD/cutestuff/cutestuff.pri)
include($$PWD/core/core.pri)
include($$PWD/irisnet/irisnet.pri)
include($$PWD/libidn/libidn.pri)

INCLUDEPATH += $$PWD
HEADERS += \
	$$PWD/IQ.h \
	$$PWD/ComponentStream.h \
	$$PWD/Stanza.h \
	$$PWD/Message.h \
	$$PWD/Presence.h
SOURCES += \
	$$PWD/IQ.cpp \
	$$PWD/StreamError.cpp \
	$$PWD/Presence.cpp \
	$$PWD/ComponentStream.cpp \
	$$PWD/Message.cpp \
	$$PWD/Stanza.cpp
