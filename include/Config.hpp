#ifndef CONFIG_HPP
# define CONFIG_HPP

# include <map>
# include <vector>
# include <iostream>
# include <string>
# include <fstream>
# include <sstream>
# include <stdexcept>
# include <poll.h>


// Structure pour un bloc "location"
struct Location {
    std::string path;
    std::string index;
    bool autoindex;
    std::vector<std::string> methods;

    void display() const;
};

class Config {
private:
    // int listen;
    std::vector<int> listen_ports;
    std::string serverName;
    std::string root;
    std::string globalIndex;
    int max_body;
    std::vector<int> server_sockets;
    std::map<int, std::string> error_pages;
    std::vector<Location> locations;
    int serverIndex;

public:
    
    bool running;
    pollfd pfd;
    Config();
    Config(const Config& other);
    Config& operator=(const Config& other);
    ~Config();

    // Getters et setters

    // void setListen(int port);
    // int getListen() const;

    void addListenPort(int port);
    const std::vector<int>& getListenPorts() const;

    void setServerName(const std::string& name);
    const std::string& getServerName() const;

    void setRoot(const std::string& r);
    const std::string& getRoot() const;

    void setIndex(const std::string& idx);
    const std::string& getIndex() const;

    void setMaxBody(long long size);
    long long getMaxBody() const;

    void setServerSocket(int socket);
    const std::vector<int>& getServerSockets() const;

    void setServerIndex(int index);
    int getServerIndex() const;

    void addErrorPage(int code, const std::string& path);
    const std::map<int, std::string>& getErrorPages() const;

    void addLocation(const Location& loc);
    const std::vector<Location>& getLocations() const;

    void display() const;

    void acceptConnections();

    void stop();

    // Lecture de configuration depuis un fichier
    static std::vector<Config> parseConfigFile(const std::string& filePath);
};

#endif // CONFIG_HPP
