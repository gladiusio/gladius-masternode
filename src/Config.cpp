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
    // read IP from TOML, call setIP()
    auto ip = toml->get_as<std::string>("ip_address").value_or("0.0.0.0");
    setIP(ip);
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
