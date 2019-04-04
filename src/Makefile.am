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

bin_PROGRAMS = masternode

masternode_LDFLAGS = --verbose -pthread -pie -Wl,-z,relro,-z,now

masternode_SOURCES = \
    CachedRoute.cpp \
    Config.cpp \
    ContentCache.cpp \
    DirectHandler.cpp \
    Masternode.cpp \
    MasternodeServer.cpp \
    NetworkState.cpp \
    ProxyHandler.cpp \
    RedirectHandler.cpp \
    RejectHandler.cpp \
    Router.cpp \
    ServiceWorker.cpp \
    ServiceWorkerHandler.cpp

masternode_LDADD = \
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


TESTS = masternode_tests

check_PROGRAMS = masternode_tests

masternode_tests_SOURCES = \
    CachedRoute.cpp \
    Config.cpp \
    ContentCache.cpp \
    DirectHandler.cpp \
    Masternode.cpp \
    MasternodeServer.cpp \
    NetworkState.cpp \
    ProxyHandler.cpp \
    RedirectHandler.cpp \
    RejectHandler.cpp \
    Router.cpp \
    ServiceWorker.cpp \
    ServiceWorkerHandler.cpp \
    tests/ConfigTests.cpp \
    tests/MasternodeTests.cpp \
    tests/NetworkStateTests.cpp \
    tests/TestRunner.cpp

masternode_tests_LDADD = \
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
    -lgtest \
    -lz 
    
masternode_tests_LDFLAGS = -pthread
