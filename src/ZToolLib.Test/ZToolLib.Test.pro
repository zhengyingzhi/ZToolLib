######################################################################
# Automatically generated by qmake (3.0) Fri Apr 22 14:06:12 2016
######################################################################
TARGET = ztoollib.test

build_pass:CONFIG(debug, debug|release) {
        BuildConf = debug
        TARGET = $$join(TARGET,,,d)
        BuildSuff = d
} else {
        BuildConf = release
        BuildSuff = ""
}


TCONFIG -= qt
QT      -= core gui

CONFIG += debug_and_release static
CONFIG += object_parallel_to_source


TEMPLATE = app




build_pass:CONFIG(debug, debug|release) {
        DEFINES += DEBUG _DEBUG ZTL_DEBUG
        DESTDIR = ../../build/debug
} else {
        DEFINES += NDEBUG
        DESTDIR = ../../build/release
}

# QMAKE_CFLAGS += -std=c99
# QMAKE_CFLAGS += -std=gnu99

INCLUDEPATH += ../

LIBS += -L$${DESTDIR} -lztoollib$${BuildSuff} -lpthread -lm -lrt -ldl


# Input
HEADERS += 

SOURCES += ZToolLib.Test.c \
    event_dispatcher_demo.c \
    producer_consumer_demo.c \
    tcp_client_demo.c \
    tcp_server_demo.c \
    threadpool_demo.c \
    trans_md_demo.c \
    udp_demo.c
