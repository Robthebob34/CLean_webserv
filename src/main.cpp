#include "Config.hpp"
#include "Servers.hpp"

int main() {
    
    std::ifstream configFile("./config/server.conf");
    if (!configFile.is_open()) {
        std::cerr << "Error: Unable to open configuration file" << std::endl;
        return 1;
    }
    Servers servers;
    std::vector<Config> configs = Config::parseConfigFile("./config/server.conf");
    for (size_t i = 0; i < configs.size(); ++i) {
        servers.addServerConfig(i, configs[i]);
    }
    try {
        servers.setup();  // Initialise les serveurs
    } catch (const std::exception& e) {
        std::cerr << "Error during server setup: " << e.what() << std::endl;
        return 1;
    }
    servers.start();
    return 0;
}