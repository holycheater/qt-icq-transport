include($$PWD/cutestuff/cutestuff.pri)
include($$PWD/core/core.pri)
include($$PWD/xmpp-ext/xmpp-ext.pri)
include($$PWD/irisnet/irisnet.pri)
include($$PWD/libidn/libidn.pri)
INCLUDEPATH += $$PWD
HEADERS += \
	$$PWD/ComponentStream.h
SOURCES += \
	$$PWD/ComponentStream.cpp \
	$$PWD/StreamError.cpp
