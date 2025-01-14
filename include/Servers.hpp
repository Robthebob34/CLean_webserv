#ifndef SERVERS_HPP
#define SERVERS_HPP

#include <map>
#include <vector>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include "Config.hpp"
#include "utils.hpp"

struct ClientInfo {
    time_t last_activity;
    std::string request;
    int cgi_state;
    std::string method;
    std::string path;
    pollfd client_pollfd;
    std::vector<unsigned char> requestBuffer;
    bool headerComplete;
    unsigned long contentLength;
    long long max_body;
};

class Servers {
private:
    std::map<int, Config> serverConfigs;
    std::vector<pollfd> poll_fds;
    std::vector<int> server_poll_fds;
    std::vector<int> client_poll_fds;

public:
    std::map<int, ClientInfo> clientSockets;


    void addServerConfig(int id, Config& config);

    void removeServerConfig(int id);

    const Config& getServerConfig(int id) const;

    void displayConfigs() const;

    void makeNonBlocking(int socket_fd);

    int createSocket(int port);

    void bindSocket(int socket_fd, int port);

    void setup();

    void initializePollFds();

    void start();

    void acceptConnections();

    //void logMessage(const std::string& level, const std::string& message);

    void add_client_to_poll(int client_fd);

    void close_connexion(int client_fd);

    void readRequest(int client_fd);

    void handleGet(int client_fd, const std::string& file_path);

    //std::string getContentType(const std::string& file_path);

    void handlePost(int client_fd);

    void handleDelete(int client_fd, const std::string& file_path);
};

#endif // SERVERS_HPP

