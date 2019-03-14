
SUBDIRS = .

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/libs -Wall -Werror -Og

TESTS = masternode_tests
check_PROGRAMS = masternode_tests

masternode_tests_SOURCES = \
    MasternodeTests.cpp \
    NetworkStateTests.cpp \
    TestRunner.cpp

masternode_tests_LDADD = \
    $(top_builddir)/libmasternode.la \
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
    -lmyhtml \
    -lz
    
masternode_tests_LDFLAGS = -pthread
