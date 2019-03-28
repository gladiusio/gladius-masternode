AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/libs \
    -Wall \
    -Wextra \
    -Wformat-security \
    -D_FORTIFY_SOURCE=2 \
    -fstack-protector \
    -fPIE

lib_LTLIBRARIES = libmasternode.la

libmasternode_la_SOURCES= \
    Cache.cpp \
    Config.cpp \
    DirectHandler.cpp \
    Masternode.cpp \
    NetworkState.cpp \
    ProxyHandler.cpp \
    RedirectHandler.cpp \
    RejectHandler.cpp \
    Router.cpp \
    ServiceWorker.cpp \
    ServiceWorkerHandler.cpp

libmasternode_la_LDFLAGS = -static -pthread -pie -Wl,-z,relro,-z,now

libmasternode_la_LIBADD = \
    -lproxygenhttpserver \
    -lproxygenlib \
    -lwangle \
    -lfolly \
    -lboost_system \
    -lboost_thread \
    -lglog \
    -lgflags \
    -lcrypto \
    -ldouble-conversion \
    -lmyhtml

bin_PROGRAMS = masternode

masternode_LDFLAGS = -pthread -pie -Wl,-z,relro,-z,now

masternode_SOURCES = \
    MasternodeServer.cpp 

masternode_LDADD = \
    libmasternode.la \
    -lproxygenhttpserver


TESTS = masternode_tests

check_PROGRAMS = masternode_tests

masternode_tests_SOURCES = \
    tests/ConfigTests.cpp \
    tests/MasternodeTests.cpp \
    tests/NetworkStateTests.cpp \
    tests/TestRunner.cpp

masternode_tests_LDADD = \
    libmasternode.la \
    -lproxygenhttpserver \
    -lgtest \
    -lz
    
masternode_tests_LDFLAGS = -pthread
