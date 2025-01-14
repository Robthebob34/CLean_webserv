#ifndef UTILS_HPP
# define UTILS_HPP

#include <string>
#include <iostream>
#include <map>
#include <iostream>
#include <sys/socket.h>
#include <fcntl.h>
#include <fstream>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <cerrno>
#include <poll.h>

std::string getFilePath(const std::string& request_path);

void sendInvalidUploadResponse(int client_fd);

void displayPollFds(const std::vector<pollfd>& poll_fds);

void displayRequestBufferAsText(const std::vector<unsigned char>& requestBuffer);

std::string getContentType(const std::string& file_path);

void logMessage(const std::string& level, const std::string& message);

bool fileExists (const std::string& f);

#endif