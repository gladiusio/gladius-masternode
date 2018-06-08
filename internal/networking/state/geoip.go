package state

import (
	"fmt"
	"os"
	"path"
	"path/filepath"

	"github.com/mholt/archiver"
	"github.com/valyala/fasthttp"

	"github.com/oschwald/geoip2-golang"
	"github.com/spf13/viper"
)

// InitGeoIP opens the GeoIP database and returns a pointer
// to it. If it does not already exist on disk, this function
// will also download the binary for it.
func InitGeoIP() (*geoip2.Reader, error) {
	geoipDir := viper.GetString("GeoIPDatabaseDir")
	// Make the geoip directory (may already exist)
	if err := os.MkdirAll(geoipDir, 0755); err != nil {
		return nil, err
	}

	// Walk the directory and look for existing database
	dbPath, err := searchForDB(geoipDir)
	if err != nil {
		return nil, err
	}

	filePath := path.Join(geoipDir, viper.GetString("GeoIPDatabaseFile"))
	// If it's not found on disk, go download the latest version
	if len(dbPath) == 0 {
		// Download the database file
		err = func() error {
			newDB, err := os.Create(filePath + ".tar.gz")
			if err != nil {
				return err
			}
			defer newDB.Close()
			statusCode, body, err := fasthttp.Get(nil, viper.GetString("GeoIPDatabaseURL"))
			if err != nil {
				return err
			}
			if statusCode != 200 {
				return fmt.Errorf("Got bad status code \"%d\" when downloading GeoIP database file", statusCode)
			}
			_, err = newDB.Write(body)
			return err
		}()
		if err != nil {
			return nil, err
		}
		// extract file contents
		err = archiver.TarGz.Open(filePath+".tar.gz", geoipDir)
		if err != nil {
			return nil, err
		}
		// Find the latest version now (tar paths may change)
		dbPath, err = searchForDB(geoipDir)
		if err != nil {
			return nil, err
		}
		if len(dbPath) == 0 {
			return nil, fmt.Errorf("Could not find GeoIP database file")
		}
	}

	// Return pointer to database reader
	db, err := geoip2.Open(dbPath)
	if err != nil {
		return nil, err
	}
	return db, nil
}

// searchForDB Walks a given file path for the GeoIP database file
func searchForDB(root string) (matchPath string, err error) {
	err = filepath.Walk(root, func(path string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}
		if !info.IsDir() && info.Name() == viper.GetString("GeoIPDatabaseFile") {
			matchPath = path
		}
		return nil
	})
	if err != nil {
		return "", err
	}
	return matchPath, nil
}
