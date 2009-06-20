include($$PWD/xmpp-core/xmpp-core.pri)
include($$PWD/xmpp-ext/xmpp-ext.pri)

INCLUDEPATH += $$PWD

HEADERS += \
        $$PWD/componentstream.h \
        $$PWD/stream.h \
        $$PWD/stream_p.h \
        $$PWD/streamerror.h
SOURCES += \
        $$PWD/componentstream.cpp \
        $$PWD/stream.cpp \
        $$PWD/streamerror.cpp
