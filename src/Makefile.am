AUTOMAKE_OPTIONS = foreign

SUBDIRS = . tests

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/libs \
    -Wall \
    -Wextra \
    -Wformat-security \
    -D_FORTIFY_SOURCE=2 \
    -fstack-protector \
    -fPIE

bin_PROGRAMS = masternode

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
    -lmyhtml \
    -lgeolite2++ \
    -lmaxminddb

masternode_LDFLAGS = -pthread -pie -Wl,-z,relro,-z,now

masternode_SOURCES = \
    MasternodeServer.cpp \
    Masternode.cpp \
    ProxyHandler.cpp \
    Cache.cpp \
    Router.cpp \
    DirectHandler.cpp \
    ServiceWorkerHandler.cpp \
    NetworkState.cpp \
    ServiceWorker.cpp \
    RedirectHandler.cpp \
    Geo.cpp \
    EdgeNode.cpp
