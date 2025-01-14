#include "utils.hpp"

std::string getFilePath(const std::string& request_path) {
    // Définir le répertoire racine des fichiers
    const std::string base_directory = "./www";

    // Si le chemin est "/", on retourne l'index par défaut
    if (request_path == "/") {
        return base_directory + "/index.html";
    }

    // Ajouter le répertoire racine au chemin de la requête
    std::string file_path = base_directory + request_path;

    // Afficher le chemin pour le débogage
    std::cout << "Resolved file path: " << file_path << std::endl;

    return file_path;
}

void sendInvalidUploadResponse(int client_fd) {
    std::string response = "HTTP/1.1 400 Bad Request\r\n"
                           "Content-Type: text/html\r\n"
                           "Content-Length: 40\r\n"
                           "\r\n"
                           "Invalid file upload";
    send(client_fd, response.c_str(), response.size(), 0);
}

void displayPollFds(const std::vector<pollfd>& poll_fds) {
    std::cout << "Contenu de poll_fds:" << std::endl;
    for (size_t i = 0; i < poll_fds.size(); ++i) {
        std::cout << "Index " << i << ":" << std::endl;
        std::cout << "  FD: " << poll_fds[i].fd << std::endl;
        std::cout << "  Events: " << poll_fds[i].events << std::endl;
        std::cout << "  Revents: " << poll_fds[i].revents << std::endl;
    }
}

void displayRequestBufferAsText(const std::vector<unsigned char>& requestBuffer) {
    std::string content(requestBuffer.begin(), requestBuffer.end());
    std::cout << "Contenu de requestBuffer (texte) :" << std::endl;
    std::cout << content << std::endl;
}

std::string getContentType(const std::string& file_path) {
    if (file_path.substr(file_path.size() - 1) == "/") return "text/html";
    if (file_path.substr(file_path.size() - 3) == ".py") return "text/html";
    if (file_path.substr(file_path.size() - 3) == ".js") return "application/javascript";
    if (file_path.substr(file_path.size() - 4) == ".css") return "text/css";
    if (file_path.substr(file_path.size() - 4) == ".jpg" || file_path.substr(file_path.size() - 5) == ".jpeg") return "image/jpeg";
    if (file_path.substr(file_path.size() - 4) == ".png") return "image/png";
    if (file_path.substr(file_path.size() - 5) == ".html") return "text/html; charset=UTF-8";
    return "multipart/form-data"; // ANCIENNEMENT text/html;
}

void logMessage(const std::string& level, const std::string& message) {
    std::cout << "[" << level << "] " << message << std::endl;
}

bool fileExists (const std::string& f) 
{
    std::cout << f << std::endl;
    std::ifstream file(f.c_str());
    return (file.good());
}