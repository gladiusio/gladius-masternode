#!/bin/bash
/geoip/geolite2pp_get_database.sh
./masternode --v=$VERBOSE_LOG_LEVEL --logtostderr=1 --tryfromenv=ip,port,ssl_port,origin_host,origin_port,protected_domain,cert_path,key_path,cache_dir,gateway_address,gateway_port,sw_path,upgrade_insecure,pool_domain,cdn_subdomain,enable_compression,enable_service_worker,max_cached_routes,enable_p2p,geoip_path,geo_ip_enabled
