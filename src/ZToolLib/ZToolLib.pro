######################################################################
# Automatically generated by qmake (3.0) Fri Apr 22 14:06:12 2016
######################################################################
TARGET = ztoollib

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


TEMPLATE = lib



build_pass:CONFIG(debug, debug|release) {
        DEFINES += DEBUG _DEBUG ZTL_DEBUG
        DESTDIR = ../../build/debug
} else {
        DEFINES += NDEBUG ZTL_NDEBUG
        DESTDIR = ../../build/release
}

QMAKE_CFLAGS += -std=c99


# Input
HEADERS += lockfreequeue.h \
    cJSON.h \
    ztl_aes.h \
    ztl_array.h \
    ztl_atomic.h \
    ztl_base64.h \
    ztl_bitset.h \
    ztl_blocking_queue.h \
    ztl_buffer.h \
    ztl_common.h \
    ztl_config.h \
    ztl_crc.h \
    ztl_crypt.h \
    ztl_dict.h \
    ztl_dyso.h \
    ztl_event_dispatcher.h \
    ztl_evtimer.h \
    ztl_evloop.h \
    ztl_evloop_private.h \
    ztl_fixapi.h \
    ztl_hash.h \
    ztl_linklist.h \
    ztl_locks.h \
    ztl_logger.h \
    ztl_malloc.h \
    ztl_map.h \
    ztl_md5.h \
    ztl_memdb.h \
    ztl_memcpy.h \
    ztl_mempool.h \
    ztl_msg_buffer.h \
    ztl_network.h \
    ztl_palloc.h \
    ztl_producer_consumer.h \
    ztl_protocol.h \
    ztl_rbtree.h \
    ztl_sha1.h \
    ztl_shm.h \
    ztl_simple_event.h \
    ztl_threadpool.h \
    ztl_times.h \
    ztl_tcp_server.h \
    ztl_threads.h \
    ztl_unit_test.h \
    ztl_utils.h \
    ztl_vector.h 

SOURCES += lockfreequeue.c \
    cJSON.c \
    siphash.c \
    ztl_aes.c \
    ztl_array.c \
    ztl_base64.c \
    ztl_blocking_queue.c \
    ztl_buffer.c \
    ztl_config.c \
    ztl_crc.c \
    ztl_crypt.c \
    ztl_dict.c \
    ztl_dyso.c \
    ztl_event_dispatcher.c \
    ztl_evtimer.c \
    ztl_evloop.c \
    ztl_evloop_epoll.c \
    ztl_evloop_private.c \
    ztl_fixapi.c \
    ztl_hash.c \
    ztl_locks.c \
    ztl_logger.c \
    ztl_malloc.c \
    ztl_map.c \
    ztl_md5.c \
    ztl_memdb.c \
    ztl_memcpy.c \
    ztl_mempool.c \
    ztl_msg_buffer.c \
    ztl_network.c \
    ztl_palloc.c \
    ztl_producer_consumer.c \
    ztl_rbtree.c \
    ztl_sha1.c \
    ztl_shm.c \
    ztl_simple_event.c \
    ztl_threadpool.c \
    ztl_tcp_server.c \
    ztl_threads.c \
    ztl_times.c \
    ztl_unit_test.c \
    ztl_utils.c \
    ztl_vector.c 
