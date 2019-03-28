#include "cpptoml.h"

#include "Config.h"

Config::Config(bool validate):
    validate_(validate) {}

// Creates a Config object based on values provided by
// a TOML configuration file.
Config::Config(const std::string& path, bool validate):
    validate_(validate)
{
    // read config file (can throw, let it bubble up)
    auto toml = cpptoml::parse_file(path);
    setIP(toml->get_qualified_as<std::string>("server.ip")
        .value_or("0.0.0.0"));
    setPort(toml->get_qualified_as<int>("server.port").value_or(80));
    //setSSLEnabled(toml->get_as<bool>("ssl_enabled").value_or(false));

}

// validates the config as a whole to check that settings
// that depend on each other are set to valid values
void validateHolistically() {

}

void Config::setIP(const std::string& ip) {
    if (validate_) {
        // validate the IP address
        // throw exception if not valid
    }
    // set value
    this->ip = ip;
}

void Config::setPort(const int port) {
    if (validate_) {
        if (port < 1 || port > 65535) {
            throw "Invalid port number: " + port;
        }
    }
    this->port = port;
}

void Config::setSSLEnabled(const bool enabled) {
    this->ssl_enabled = enabled;
}

void Config::
