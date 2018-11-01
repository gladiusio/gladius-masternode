package config

import (
	"fmt"
	"path/filepath"
	"strings"

	common "github.com/gladiusio/gladius-common/pkg/utils"
	log "github.com/sirupsen/logrus"
	"github.com/spf13/viper"
)

func SetupConfig(configFilePath string) {
	viper.SetConfigName("gladius-masternode")
	viper.AddConfigPath(configFilePath)

	// Setup env variable handling
	viper.SetEnvPrefix("MASTERNODE")
	r := strings.NewReplacer(".", "_")
	viper.SetEnvKeyReplacer(r)
	viper.AutomaticEnv()

	err := viper.ReadInConfig()
	if err != nil {
		log.Warn(fmt.Errorf("error reading config file: %s, using defaults", err))
	}

	base, err := common.GetGladiusBase()
	if err != nil {
		log.WithFields(log.Fields{
			"err": err,
		}).Warn("Couldn't get Gladius base")
	}

	// Set defaults
	ConfigOption("MaxLogLines", 1000) // Max number of log lines to keep in ram for each service

	ConfigOption("PoolEthAddress", "0xDAcd582c3Ba1A90567Da0fC3f1dBB638D9438e06")

	ConfigOption("NetworkGatewayHostname", "localhost")
	ConfigOption("NetworkGatewayPort", "3001")
	ConfigOption("NetworkGatewayProtocol", "http")

	ConfigOption("GeoIPDatabaseDir", filepath.Join(base, "geoip"))
	ConfigOption("GeoIPDatabaseFile", "GeoLite2-City.mmdb")
	ConfigOption("GeoIPDatabaseMD5URL", "http://geolite.maxmind.com/download/geoip/database/GeoLite2-City.tar.gz.md5")
	ConfigOption("GeoIPDatabaseURL", "http://geolite.maxmind.com/download/geoip/database/GeoLite2-City.tar.gz")

	// Setup logging level
	switch loglevel := viper.GetString("LogLevel"); loglevel {
	case "debug":
		log.SetLevel(log.DebugLevel)
	case "warning":
		log.SetLevel(log.WarnLevel)
	case "info":
		log.SetLevel(log.InfoLevel)
	case "error":
		log.SetLevel(log.ErrorLevel)
	default:
		log.SetLevel(log.InfoLevel)
	}
}

func ConfigOption(key string, defaultValue interface{}) string {
	viper.SetDefault(key, defaultValue)

	return key
}
