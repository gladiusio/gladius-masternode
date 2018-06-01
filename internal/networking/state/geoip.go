package state

import (
	"fmt"
	"os"
	"path"

	"github.com/valyala/fasthttp"

	"github.com/mholt/archiver"
	"github.com/oschwald/geoip2-golang"
	"github.com/spf13/viper"
)

func InitGeoIP() (*geoip2.Reader, error) {
	geoipDir := viper.GetString("GeoIPDatabaseDir")
	// Make the geoip directory (may already exist)
	if err := os.MkdirAll(geoipDir, 0755); err != nil {
		return nil, err
	}

	// Download md5 of latest database version

	// Walk the directory and look for database that matches the md5

	// If it's found, open the database

	// Check to see if the database file exists
	filePath := path.Join(geoipDir, viper.GetString("GeoIPDatabaseFile"))
	file, err := os.Open(filePath)

	if err != nil {
		if os.IsNotExist(err) {
			// Download the database file if it's not there
			out, err := os.Create(filePath)
			if err != nil {
				return nil, err
			}
			defer out.Close()
			statusCode, body, err := fasthttp.Get(nil, "http://geolite.maxmind.com/download/geoip/database/GeoLite2-City.tar.gz")
			if err != nil {
				return nil, err
			}
			if statusCode != 200 {
				return nil, fmt.Errorf("Got bad status code \"%d\" when downloading GeoIP database file", statusCode)
			}
			_, err = out.Write(body)
			if _, err := out.Write(body); err != nil {
				return nil, err
			}
			// extract file contents
			if err := archiver.TarGz.Open(filePath, geoipDir); err != nil {
				return nil, err
			}
			//
		} else {
			return nil, err
		}
	}
	defer file.Close()

	// Return pointer to database reader
	db, err := geoip2.Open(filePath)
	if err != nil {
		return nil, err
	}
	return db, nil
}
