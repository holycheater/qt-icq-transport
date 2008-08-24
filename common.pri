CONFIG += crypto

# Iris Debug output
DEFINES += XMPP_DEBUG

CONFIG += iris_bundle
include(iris/iris.pri)

include(icq/icq.pri)
