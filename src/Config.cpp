#include "Config.hpp"


// Fonction utilitaire pour supprimer les espaces en début et en fin de chaîne
void trim(std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r\f\v");
    size_t last = str.find_last_not_of(" \t\n\r\f\v");
    if (first != std::string::npos && last != std::string::npos) {
        str = str.substr(first, (last - first + 1));
    } else {
        str.clear();
    }
}

// Méthodes de la structure Location
void Location::display() const {
    std::cout << "  Location Path: " << path << std::endl;
    std::cout << "    Index: " << index << std::endl;
    std::cout << "    Autoindex: " << (autoindex ? "on" : "off") << std::endl;
    std::cout << "    Allowed Methods: ";
    for (size_t i = 0; i < methods.size(); ++i) {
        std::cout << methods[i] << (i + 1 < methods.size() ? ", " : "");
    }
    std::cout << std::endl;
}

// Constructeur par défaut
Config::Config() : listen_ports(0), max_body(0) {}

// Constructeur par copie
Config::Config(const Config& other)
    : listen_ports(other.listen_ports),
      serverName(other.serverName),
      root(other.root),
      globalIndex(other.globalIndex),
      max_body(other.max_body),
      error_pages(other.error_pages),
      locations(other.locations) {}

// Opérateur d'affectation
Config& Config::operator=(const Config& other) {
    if (this != &other) {
        listen_ports = other.listen_ports;
        serverName = other.serverName;
        root = other.root;
        globalIndex = other.globalIndex;
        max_body = other.max_body;
        error_pages = other.error_pages;
        locations = other.locations;
    }
    return *this;
}

// Destructeur
Config::~Config() {}

// Getters et setters

void Config::addListenPort(int port) { listen_ports.push_back(port);}
const std::vector<int>& Config::getListenPorts() const { return listen_ports; }

void Config::setServerName(const std::string& name) { serverName = name; }
const std::string& Config::getServerName() const { return serverName; }

void Config::setRoot(const std::string& r) { root = r; }
const std::string& Config::getRoot() const { return root; }

void Config::setIndex(const std::string& idx) { globalIndex = idx; }
const std::string& Config::getIndex() const { return globalIndex; }

void Config::setMaxBody(long long size) { max_body = size; }
long long Config::getMaxBody() const { return max_body; }

void Config::setServerSocket(int socket) { server_sockets.push_back(socket) ; }
const std::vector<int>& Config::getServerSockets() const { return server_sockets; }

void Config::setServerIndex(int index) { serverIndex = index; }
int Config::getServerIndex() const { return serverIndex; }

void Config::addErrorPage(int code, const std::string& path) {
    error_pages[code] = path;
}
const std::map<int, std::string>& Config::getErrorPages() const {
    return error_pages;
}

void Config::addLocation(const Location& loc) {
    locations.push_back(loc);
}
const std::vector<Location>& Config::getLocations() const {
    return locations;
}

void Config::stop() {
    running = false;
    //logMessage("INFO", "Server is stopping...");
}


void Config::display() const {
    //std::cout << "Server listening on port: " << listen_ports << std::endl;
    std::cout << "Server name: " << serverName << std::endl;
    std::cout << "Root directory: " << root << std::endl;
    std::cout << "max Body: " << max_body << std::endl;
    std::cout << "Global Index: " << globalIndex << std::endl;
    
    std::cout << "Error pages:" << std::endl;
    for (std::map<int, std::string>::const_iterator it = error_pages.begin(); it != error_pages.end(); ++it) {
        const int& key = it->first;
        const std::string& value = it->second;
        std::cout << "  " << key << ": " << value << std::endl;
    }

    std::cout << "Locations:" << std::endl;
    for (std::vector<Location>::const_iterator it = locations.begin(); it != locations.end(); ++it) {
        const Location& loc = *it;
        loc.display();
    }
}

// Méthode de parsing
std::vector<Config> Config::parseConfigFile(const std::string& filePath) {

    std::vector<Config> servers;
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Impossible d'ouvrir le fichier: " + filePath);
    }

    Config currentConfig;
    Location currentLocation;
    std::string line;
    bool inServerBlock = false;
    bool inLocationBlock = false;

    while (std::getline(file, line)) {
        trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }

        if (line == "server {") {
            inServerBlock = true;
            currentConfig = Config();
        } else if (line == "}") {
            if (inLocationBlock) {
                currentConfig.addLocation(currentLocation);
                inLocationBlock = false;
            } else if (inServerBlock) {
                servers.push_back(currentConfig);
                inServerBlock = false;
            }
        } else if (inServerBlock) {
            if (line.rfind("location", 0) == 0) {
                inLocationBlock = true;
                std::istringstream ss(line);
                std::string keyword, path;
                ss >> keyword >> path;
                currentLocation = Location();
                currentLocation.path = path;
            } else if (inLocationBlock) {
                if (line.rfind("index", 0) == 0) {
                    currentLocation.index = line.substr(6);
                } else if (line.rfind("autoindex", 0) == 0) {
                    currentLocation.autoindex = (line.substr(10) == "on");
                } else if (line.rfind("allowed_methods", 0) == 0) {
                    std::istringstream ss(line.substr(16));
                    std::string method;
                    while (ss >> method) {
                        currentLocation.methods.push_back(method);
                    }
                }
            } else {
                // Dans Config.cpp, méthode parseConfigFile
                if (line.rfind("listen", 0) == 0) {
                    currentConfig.addListenPort(std::stoi(line.substr(7)));
                } else if (line.rfind("serverName", 0) == 0) {
                    currentConfig.setServerName(line.substr(12));
                } else if (line.rfind("root", 0) == 0) {
                    currentConfig.setRoot(line.substr(5));
                } else if (line.rfind("index", 0) == 0) {
                    currentConfig.setIndex(line.substr(6));
                } else if (line.rfind("max_body", 0) == 0) {
                    currentConfig.setMaxBody(std::stoll(line.substr(9)));
                } else if (line.rfind("error_pages", 0) == 0) {
                    std::istringstream ss(line.substr(12));
                    int code;
                    std::string path;
                    ss >> code >> path;
                    currentConfig.addErrorPage(code, path);
                }
            }
        }
    }

    file.close();
    return servers;
}