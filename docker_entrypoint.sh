#!/bin/bash
/geoip/geolite2pp_get_database.sh
./masternode --v=$VERBOSE_LOG_LEVEL --logtostderr=1 --config=$CONFIG_PATH
