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


# INSTALLATION
isEmpty(install_base) {
	install_base = "/usr"
}

target.path = $$install_base/bin

examples.path = $$install_base/share/doc/qt-icq-transport
examples.files = examples/*

INSTALLS += target examples

message("Binaries install path: " $$target.path)
message("Examples install path: " $$examples.path)
