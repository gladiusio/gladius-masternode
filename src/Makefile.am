AUTOMAKE_OPTIONS = foreign
bin_PROGRAMS = masternode
masternode_LDADD = -lproxygenhttpserver -lproxygenlib -lwangle -lfolly -lglog
masternode_LDFLAGS = -pthread
masternode_SOURCES = masternode.cpp ProxyHandler.cpp ProxyHandler.h Cache.h
