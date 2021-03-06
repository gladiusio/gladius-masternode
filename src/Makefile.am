AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/libs \
    -Wall \
    -Wextra \
    -Wno-unused-parameter \
    -Wformat-security \
    -D_FORTIFY_SOURCE=2 \
    -fstack-protector \
    -fPIE

lib_LTLIBRARIES = libmasternode.la

libmasternode_la_SOURCES= \
    Masternode.cpp \
    ProxyHandler.cpp \
    Cache.cpp \
    Router.cpp \
    DirectHandler.cpp \
    ServiceWorkerHandler.cpp \
    NetworkState.cpp \
    ServiceWorker.cpp \
    RedirectHandler.cpp \
    RejectHandler.cpp \
    Geo.cpp \
    EdgeNode.cpp

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
    -lmyhtml \
    -lgeolite2++ \
    -lmaxminddb

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
    tests/MasternodeTests.cpp \
    tests/NetworkStateTests.cpp \
    tests/TestRunner.cpp \
    tests/GeoTests.cpp \
    tests/EdgeNodeTests.cpp

masternode_tests_LDADD = \
    libmasternode.la \
    -lproxygenhttpserver \
    -lgtest \
    -lz
    
masternode_tests_LDFLAGS = -pthread
