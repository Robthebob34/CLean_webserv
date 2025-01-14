#include "Servers.hpp"
#include "utils.hpp"
#include <sys/socket.h>
#include <poll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <string>

// Ajouter une configuration
void Servers::addServerConfig(int id, Config& config) {
    config.setServerIndex(id);
    serverConfigs[id] = config;
}

// Supprimer une configuration par ID
void Servers::removeServerConfig(int id) {
    serverConfigs.erase(id);
}

// Récupérer une configuration par ID
const Config& Servers::getServerConfig(int id) const {
    std::map<int, Config>::const_iterator it = serverConfigs.find(id);
    if (it != serverConfigs.end()) {
        return it->second;
    }
    throw std::runtime_error("Server configuration not found for ID: " + std::to_string(id));
}

// Afficher les configurations
void Servers::displayConfigs() const {
    for (std::map<int, Config>::const_iterator it = serverConfigs.begin(); it != serverConfigs.end(); ++it) {
        std::cout << "Server ID: " << it->first << std::endl;
        // Ajoutez ici l'affichage de Config si nécessaire
    }
}
// Helper function to make a socket non-blocking
void Servers::makeNonBlocking(int socket_fd) {
    int flags = fcntl(socket_fd, F_GETFL, 0);
    if (flags == -1) {
        throw std::runtime_error("Failed to get socket flags: " + std::string(strerror(errno)));
    }
    if (fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw std::runtime_error("Failed to set non-blocking mode: " + std::string(strerror(errno)));
    }
}

// Function to create a socket
int Servers::createSocket(int port) {
    (void) port;
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
    }

    int opt = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        close(socket_fd);
        throw std::runtime_error("Failed to set socket options: " + std::string(strerror(errno)));
    }

    return socket_fd;
}

// Function to bind a socket to a port
void Servers::bindSocket(int socket_fd, int port) {
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(port);

    if (bind(socket_fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == -1) {
        close(socket_fd);
        throw std::runtime_error("Failed to bind socket: " + std::string(strerror(errno)));
    }

    if (listen(socket_fd, SOMAXCONN) == -1) {
        close(socket_fd);
        throw std::runtime_error("Failed to listen on socket: " + std::string(strerror(errno)));
    }
}

void Servers::setup() {
    std::cout << "Number of server(s): " << serverConfigs.size() << std::endl;

    for (std::map<int, Config>::iterator it = serverConfigs.begin(); it != serverConfigs.end(); ++it) {
        Config& config = it->second;
        const std::vector<int>& ports = config.getListenPorts();

        for (std::vector<int>::const_iterator port_it = ports.begin(); port_it != ports.end(); ++port_it) {
            int port = *port_it;
            printf("port: %d\n", port);
            int socket_fd = createSocket(port);
            printf("socket_fd: %d\n", socket_fd);
            makeNonBlocking(socket_fd);
            bindSocket(socket_fd, port);

            config.setServerSocket(socket_fd); // Stocker le socket dans la config
            std::cout << "Server setup on port " << port << " with socket FD " << socket_fd << std::endl;
        }
    }
}


void Servers::initializePollFds() {
    this->poll_fds.clear();
    for (std::map<int, Config>::iterator it = serverConfigs.begin(); it != serverConfigs.end(); ++it) {
        Config& config = it->second;
        const std::vector<int>& ports = config.getListenPorts();
        const std::vector<int>& serverSockets = config.getServerSockets();
        std::vector<int>::const_iterator socket_it = serverSockets.begin();
        for (std::vector<int>::const_iterator port_it = ports.begin(); port_it != ports.end(); ++port_it, ++socket_it) {
            //int port = *port_it;
            int socket_fd = *socket_it;
            struct pollfd server_pollfd;
            server_pollfd.fd = socket_fd; // Obtenir le FD pour ce port
            printf("sockets: %d\n", server_pollfd.fd);
            server_pollfd.events = POLLIN;
            server_pollfd.revents = 0;

            poll_fds.push_back(server_pollfd);          // Ajouter au tableau poll
            server_poll_fds.push_back(server_pollfd.fd); // Enregistrer le FD du socket serveur
        }
    }
    for (std::map<int, ClientInfo>::iterator it = clientSockets.begin(); it != clientSockets.end(); ++it) {
        it->second.client_pollfd.fd = it->first;
        it->second.client_pollfd.events = POLLIN | POLLOUT;
        it->second.client_pollfd.revents = 0;
        // it->second.serverIndex = 
        poll_fds.push_back(it->second.client_pollfd);
        client_poll_fds.push_back(it->second.client_pollfd.fd);
    }
}

std::string trim_cgi_param(std::string str)
{
    if(str.find_first_of('?') == std::string::npos)
        return str;
    else
    {
        int i = str.find('?');
        return str.substr(0, i);
    }
}

void Servers::handleGet(int client_fd, const std::string& file_path) {
    // Extraire le chemin réel
    std::string real_file_path = trim_cgi_param(file_path);
    std::ifstream file(real_file_path, std::ios::binary | std::ios::ate);
    printf("FILE path: %s\n", file_path.c_str());
    if (!file.is_open()) {
        std::cerr << "Error: Failed to open file: " << real_file_path << std::endl;
        //sendErrorResponse(client_fd, 404); // Envoyer une erreur 404
        return;
    }

    // Obtenir la taille du fichier
    std::streamsize file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    // Lire le contenu du fichier dans un vecteur
    std::vector<unsigned char> file_content(file_size);
    if (!file.read(reinterpret_cast<char*>(file_content.data()), file_size)) {
        std::cerr << "Error: Failed to read file: " << real_file_path << std::endl;
        //sendErrorResponse(client_fd, 500); // Envoyer une erreur 500
        return;
    }
    file.close();

    // Construire l'en-tête HTTP
    std::string content_type = getContentType(real_file_path);
    std::string response_header =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: " + content_type + "\r\n"
        "Content-Length: " + std::to_string(file_size) + "\r\n"
        "\r\n";

    // Envoyer l'en-tête HTTP
    if (send(client_fd, response_header.c_str(), response_header.size(), 0) < 0) {
        std::cerr << "Error: Failed to send response header to client FD: " << client_fd << std::endl;
        return;
    }

    // Envoyer le contenu du fichier
    if (send(client_fd, file_content.data(), file_content.size(), 0) < 0) {
        std::cerr << "Error: Failed to send file content to client FD: " << client_fd << std::endl;
    }
}


void Servers::readRequest(int client_fd) {
    unsigned char buffer[4096];
    memset(buffer, 0, sizeof(buffer));
    ClientInfo &client = clientSockets[client_fd];
    ssize_t bytes_read;
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
        if (bytes_read < 0) {
            perror("recv failed");
            break;
        }
        else if (bytes_read == 0) {
            break;
        }
        client.requestBuffer.insert(client.requestBuffer.end(), buffer, buffer + bytes_read);
    }
    logMessage("DEBUG", "Bytes read from FD " + std::to_string(client_fd) + ": " + std::to_string(bytes_read));
    std::string requestHeader(client.requestBuffer.begin(), client.requestBuffer.end());
    if(requestHeader.find("\r\n\r\n") != std::string::npos) {
        std::cout << "Requête complète reçue pour le client FD: " << client_fd << std::endl;

        // Extraire les informations de la requête
        size_t method_end = requestHeader.find(' ');
        if (method_end != std::string::npos) {
            client.method = requestHeader.substr(0, method_end);
            size_t path_end = requestHeader.find(' ', method_end + 1);
            if (path_end != std::string::npos) {
                client.path = requestHeader.substr(method_end + 1, path_end - method_end - 1);
            }
        }
        if (static_cast<long long>(client.requestBuffer.size()) > clientSockets[client_fd].max_body) {
            std::cerr << "Error: BODY TOO LARGE\n" << std::endl;
            sendInvalidUploadResponse(client_fd);
            close_connexion(client_fd);
            return;
        }
        // Gérer différentes méthodes HTTP
        if (client.method == "GET") {
            handleGet(client_fd, getFilePath(client.path));
        } else if (client.method == "POST") {
            handlePost(client_fd);
        } else if (client.method == "DELETE") {
            handleDelete(client_fd, client.path);
        } else {
            //sendError(client_fd, "501 Not Implemented");
        }
        // Vider le buffer après traitement
        client.requestBuffer.clear();
    }
}



void Servers::acceptConnections() {
    while (true) {
        int poll_ret = poll(poll_fds.data(), poll_fds.size(), 1000);
        if (poll_ret < 0) {
            logMessage("ERROR", std::string("Poll error: ") + strerror(errno));
            //close_all_fd();
            exit(1);
        }
        for (size_t i = 0; i < poll_fds.size(); i++) {
            int fd = poll_fds[i].fd;
            long long bodyMax = -1;
            for (std::map<int, Config>::iterator it = serverConfigs.begin(); it != serverConfigs.end(); ++it) {
                const std::vector<int>& sockets = it->second.getServerSockets(); // Récupérer les sockets du serveur
                if (std::find(sockets.begin(), sockets.end(), fd) != sockets.end()) {
                    // Si le FD correspond à l'un des sockets
                    bodyMax = it->second.getMaxBody();
                    break;
                }
            }
            if (poll_fds[i].revents & POLLHUP) {
                logMessage("WARNING", "Poll close connection event on fd [" + std::to_string(fd) + "]...");
                close_connexion(fd);
            }
            else if (poll_fds[i].revents & POLLIN) {

                if (std::find(server_poll_fds.begin(), server_poll_fds.end(), fd) != server_poll_fds.end()) {
                    struct sockaddr_in client_address;
                    socklen_t client_addr_len = sizeof(client_address);
                    int client_fd = accept(fd, (struct sockaddr*)&client_address, &client_addr_len);

                    if (client_fd < 0) {
                        if (errno == EWOULDBLOCK || errno == EAGAIN)
                            continue;
                        logMessage("ERROR", std::string("Failed to accept connection: ") + strerror(errno));
                        continue;
                    }
                    makeNonBlocking(client_fd);
                    ClientInfo client;
                    client.max_body = bodyMax;
                    client.last_activity = time(NULL);
                    client.client_pollfd.fd = client_fd;
                    client.client_pollfd.events = POLLIN | POLLOUT;
                    client.client_pollfd.revents = 0;
                    clientSockets[client_fd] = client;
                    poll_fds.push_back(client.client_pollfd);
                    client_poll_fds.push_back(client.client_pollfd.fd);
                    logMessage("INFO", "New client connected on fd: " + std::to_string(client_fd));
                }
                else if (clientSockets.find(fd) != clientSockets.end()) {
                    if (clientSockets[fd].cgi_state == 2) {
                        //read_cgi_output(fd);
                    } else {
                        readRequest(fd);
                    }
                }
            }
            else if (poll_fds[i].revents & POLLOUT) {
                if (clientSockets.find(fd) != clientSockets.end()) {
                    std::string method = clientSockets[fd].method;
                    if (method == "GET" || clientSockets[fd].cgi_state == 1) {
                        handleGet(fd, getFilePath(clientSockets[fd].path));
                    } else if (method == "POST") {
                        handlePost(fd);
                    } else if (method == "DELETE") {
                        handleDelete(fd, clientSockets[fd].path);
                    }
                }
                clientSockets[fd].method.erase();
            }
        }
        //Check_TimeOut();
    }
}

void Servers::add_client_to_poll(int client_fd) {
    ClientInfo client;
    client.last_activity = time(NULL);
    client.client_pollfd.fd = client_fd;
    client.client_pollfd.events = POLLIN | POLLOUT;
    client.client_pollfd.revents = 0;
    clientSockets[client_fd] = client;
    logMessage("INFO", "New Client Connection on fd " + std::to_string(client_fd));
}

void Servers::close_connexion(int client_fd) {
    if (clientSockets.find(client_fd) != clientSockets.end()) {
        close(client_fd);
        clientSockets.erase(client_fd);
        logMessage("INFO", "Client fd [" + std::to_string(client_fd) + "] closed and removed.");
    }
}

void Servers::start() {
    try {
        initializePollFds();
        acceptConnections();
    } catch (const std::exception& e) {
        logMessage("ERROR", std::string("Server encountered an error: ") + e.what());
        //close_all_fd();
        exit(1);
    }
}

void Servers::handlePost(int client_fd) {
    ClientInfo& client = clientSockets[client_fd];
    std::string request(client.requestBuffer.begin(), client.requestBuffer.end());

    // Attempt to find the boundary
    size_t boundary_pos = request.find("boundary=");
    std::string boundary;
    if (boundary_pos != std::string::npos) {
        boundary = "--" + request.substr(boundary_pos + 9, request.find("\r\n", boundary_pos) - (boundary_pos + 9));
    }
    //if (request.length() != 0)
        //printf("REQUEST:\n%s\n ", request.c_str());
    size_t start = boundary.empty() ? 0 : request.find(boundary);
    while (request.length() != 0 && (start != std::string::npos || (start == 0 && boundary.empty()))) {
        if (!boundary.empty()) {
            start += boundary.length();
        }
        size_t end = boundary.empty() ? std::string::npos : request.find(boundary, start);

        // Extract Content-Disposition header
        size_t disposition_start = request.find("Content-Disposition:", start);
        if (disposition_start == std::string::npos || (!boundary.empty() && disposition_start >= end)) {
            //std::cerr << "Error: Content-Disposition not found in POST request." << std::endl;
            //sendInvalidUploadResponse(client_fd);
            return;
        }

        size_t filename_start = request.find("filename=\"", disposition_start) + 10;
        size_t filename_end = request.find("\"", filename_start);
        std::string filename;
        if (filename_start != std::string::npos && filename_end != std::string::npos) {
            filename = request.substr(filename_start, filename_end - filename_start);
        } else {
            filename = "uploaded_file";
        }

        std::string file_path = "./www/uploads/" + filename;

        // Extract the file content
        size_t file_start = request.find("\r\n\r\n", disposition_start) + 4;
        if (file_start == std::string::npos || (!boundary.empty() && file_start >= end)) {
            std::cerr << "Error: File content not found in POST request." << std::endl;
            sendInvalidUploadResponse(client_fd);
            return;
        }

        size_t file_end = end != std::string::npos ? end - 2 : request.size();
        std::string file_content = request.substr(file_start, file_end - file_start);

        // Write the file to disk
        printf("file to open: %s\n",file_path.c_str());
        int fd = open(file_path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
        if (fd < 0) {
            std::cerr << "Error: Failed to open file for writing." << std::endl;
            sendInvalidUploadResponse(client_fd);
            return;
        }

        if (write(fd, file_content.c_str(), file_content.size()) < 0) {
            std::cerr << "Error: Failed to write to file." << std::endl;
            close(fd);
            sendInvalidUploadResponse(client_fd);
            return;
        }
        
        //close(fd);
        // Send success response to the client
        std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\nFile uploaded successfully.";
        if(send(client_fd, response.c_str(), response.size(), 0) < 0){
             std::cerr << "Error: Failed to send response for POST to client FD: " << client_fd << std::endl;
        }

        std::cout << "File " << filename << " uploaded successfully." << std::endl;

        if (boundary.empty()) {
            break;
        }
        start = end;
    }
    close_connexion(client_fd);
}

void Servers::handleDelete(int client_fd, const std::string& file_path)
{
    std::string response;
    std::string completePath = "www/uploads" + file_path;  // Concaténation de chaînes de type std::string
   
    if (fileExists(completePath))
    {
        if (std::remove(completePath.c_str()) == 0)
        {
           
            response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
                       "File deleted successfully";
        }
        else
        {
            response = "HTTP/1.1 500 Not Found\r\nContent-Type: text/html\r\n\r\n"
                   "Internal serveur error";
        }
    }
    else
    {
        response = "HTTP/1.1 404 Server Error\r\nContent-Type: text/html\r\n\r\n"
                       "FILE NOT FOUND";
    }
    send(client_fd, response.c_str(), response.size(), 0);
    clientSockets[client_fd].request.clear();
    clientSockets[client_fd].client_pollfd.events = POLLIN;
}
