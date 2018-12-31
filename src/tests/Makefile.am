
SUBDIRS = .

AM_CPPFLAGS = -I$(top_srcdir)

TESTS = masternode_tests
check_PROGRAMS = masternode_tests
masternode_tests_SOURCES = MasternodeTests.cpp $(top_builddir)/ProxyHandlerFactory.cpp $(top_builddir)/ProxyHandler.cpp $(top_builddir)/Cache.cpp $(top_builddir)/DirectHandler.cpp
masternode_tests_LDADD = -lproxygenhttpserver -lproxygenlib -lwangle -lfolly -lboost_system -lboost_thread -lglog -lgflags -lgtest -lcrypto -lssl -ldouble-conversion
masternode_tests_LDFLAGS = -pthread
