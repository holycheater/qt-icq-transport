TARGET = qt-icq-transport
TEMPLATE = app

include(common.pri)
include(icq/icq.pri)
include(shark/shark.pri)
include(src/src.pri)

MOC_DIR = .moc
OBJECTS_DIR = .obj

QMAKE_DISTCLEAN += \
	$$PWD/debug \
	$$PWD/release \
	$$PWD/.moc \
    	$$PWD/.obj

QMAKE_DEL_FILE = rm -rf
