package state

import (
	"archive/tar"
	"bytes"
	"compress/gzip"
	"crypto/md5"
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"path"
	"path/filepath"
	"strings"

	"github.com/gladiusio/gladius-masternode/internal/http"
	"github.com/valyala/fasthttp"

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
	latestMD5, err := http.GetBytes("http://geolite.maxmind.com/download/geoip/database/GeoLite2-City.tar.gz.md5")
	if err != nil {
		return nil, err
	}

	needsNewVersion := true
	// Walk the directory and look for database that matches the md5
	err = filepath.Walk(geoipDir, func(path string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}
		if !info.IsDir() && info.Name() == viper.GetString("GeoIPDatabaseFile") {
			// see if the md5 of this file matches the latest version
			data, err := ioutil.ReadFile(path)
			if err != nil {
				return err
			}
			thisMD5 := md5.Sum(data)
			sum := thisMD5[:]
			if bytes.Equal(latestMD5, sum) {
				// Latest version of database is already on disk
				needsNewVersion = false
			}
		}
		return nil
	})
	if err != nil {
		return nil, err
	}
	filePath := path.Join(geoipDir, viper.GetString("GeoIPDatabaseFile"))
	// If it's not found on disk, go download the latest version
	if needsNewVersion {
		// Download the database file if it's not there
		err = func() error {
			newDB, err := os.Create(filePath + ".tar.gz")
			if err != nil {
				return err
			}
			defer newDB.Close()
			statusCode, body, err := fasthttp.Get(nil, "http://geolite.maxmind.com/download/geoip/database/GeoLite2-City.tar.gz")
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
		// todo
		err = extractGeoIP(filePath+".tar.gz", geoipDir)
		if err != nil {
			return nil, err
		}
	}

	// Return pointer to database reader
	db, err := geoip2.Open(filePath)
	if err != nil {
		return nil, err
	}
	return db, nil
}

func extractGeoIP(tgzPath, targetDir string) error {
	bundlePath := strings.TrimSuffix(tgzPath, ".gz")

	err := func() error {
		// Uncompress the database
		reader, err := os.Open(tgzPath)
		if err != nil {
			return err
		}
		defer reader.Close()

		archive, err := gzip.NewReader(reader)
		if err != nil {
			return err
		}
		defer archive.Close()

		writer, err := os.Create(bundlePath)
		if err != nil {
			return err
		}
		defer writer.Close()

		_, err = io.Copy(writer, archive)
		return err
	}()
	if err != nil {
		return err
	}

	// Untar the archive
	reader, err := os.Open(bundlePath)
	if err != nil {
		return err
	}
	defer reader.Close()
	tarReader := tar.NewReader(reader)
	for {
		header, err := tarReader.Next()
		if err == io.EOF {
			break
		} else if err != nil {
			return err
		}
		path := filepath.Join(targetDir, header.Name)
		info := header.FileInfo()
		if info.IsDir() {
			if err = os.MkdirAll(path, info.Mode()); err != nil {
				return err
			}
			continue
		}
		file, err := os.OpenFile(path, os.O_CREATE|os.O_TRUNC|os.O_WRONLY, info.Mode())
		if err != nil {
			return err
		}
		defer file.Close()
		_, err = io.Copy(file, tarReader)
		if err != nil {
			return err
		}
	}

	return err
}
