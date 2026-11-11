#pragma once
#include <cstdint>
#include <string>
#include <string_view>

enum class WaitStatus { Readable, Timeout, Interrupted, Error };

enum class ReadStatus { Data, Closed, Error };

WaitStatus wait_readable(int fd, int timeout);

ReadStatus read_some(int fd, std::string &buffer);

bool send_all(int fd, std::string_view data);

bool stop_requested();

bool install_signal_handlers();

int create_listen_socket(std::uint16_t port);