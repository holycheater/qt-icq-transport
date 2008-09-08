include($$PWD/cutestuff/cutestuff.pri)
include($$PWD/core/core.pri)
include($$PWD/irisnet/irisnet.pri)
include($$PWD/libidn/libidn.pri)
INCLUDEPATH += $$PWD
HEADERS += \
	$$PWD/ComponentStream.h \
	$$PWD/ServiceDiscovery.h
SOURCES += \
	$$PWD/ComponentStream.cpp \
	$$PWD/StreamError.cpp \
	$$PWD/ServiceDiscovery.cpp
