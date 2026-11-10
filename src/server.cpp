#include <cerrno>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

int create_listen_socket(std::uint16_t port) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);

  if (fd == -1) {
    int error_code = errno;
    std::string error_message = strerror(error_code);
    std::cerr << "socket failed: " << error_message << std::endl;
    return -1;
  }

  int opt = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
    int error_code = errno;
    close(fd);
    std::string error_message = strerror(error_code);
    std::cerr << "setsockopt failed: " << error_message << std::endl;
    return -1;
  }

  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  address.sin_port = htons(port);

  if (bind(fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == -1) {
    int error_code = errno;
    close(fd);
    std::string error_message = strerror(error_code);
    std::cerr << "bind failed on port " << port << ": " << error_message
              << std::endl;
    return -1;
  }

  if (listen(fd, SOMAXCONN) == -1) {
    int error_code = errno;
    close(fd);
    std::string error_message = strerror(error_code);
    std::cerr << "listen failed: " << error_message << std::endl;
    return -1;
  }

  return fd;
}