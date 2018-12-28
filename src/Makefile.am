AUTOMAKE_OPTIONS = foreign

SUBDIRS = . tests

bin_PROGRAMS = masternode
masternode_LDADD = -lproxygenhttpserver -lproxygenlib -lwangle -lfolly -lboost_system -lboost_thread -lglog -lgflags -lcrypto -ldouble-conversion
masternode_LDFLAGS = -pthread
masternode_SOURCES = Masternode.h Masternode.cpp ProxyHandler.cpp ProxyHandler.h Cache.h Cache.cpp ProxyHandlerFactory.h ProxyHandlerFactory.cpp httplib.h NetworkState.h MasternodeConfig.h
