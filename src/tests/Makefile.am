
SUBDIRS = .

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/libs

TESTS = masternode_tests
check_PROGRAMS = masternode_tests

masternode_tests_SOURCES = \
    MasternodeTests.cpp \
    NetworkStateTests.cpp \
    TestRunner.cpp \
    $(top_srcdir)/Masternode.cpp \
    $(top_srcdir)/Router.cpp \
    $(top_srcdir)/ProxyHandler.cpp \
    $(top_srcdir)/Cache.cpp \
    $(top_srcdir)/DirectHandler.cpp \
    $(top_srcdir)/ServiceWorkerHandler.cpp \
    $(top_srcdir)/NetworkState.cpp \
    $(top_srcdir)/ServiceWorker.cpp \
    $(top_srcdir)/RedirectHandler.cpp

masternode_tests_LDADD = \
    -lproxygenhttpserver \
    -lproxygenlib \
    -lwangle \
    -lfolly \
    -lboost_system \
    -lboost_thread \
    -lglog \
    -lgflags \
    -lgtest \
    -lcrypto \
    -lssl \
    -ldouble-conversion \
    -lmyhtml
    
masternode_tests_LDFLAGS = -pthread
