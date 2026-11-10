#pragma once
#include <cstdint>

bool stop_requested();

bool install_signal_handlers();

int create_listen_socket(std::uint16_t port);