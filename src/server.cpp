#include "server.h"
#include <atomic>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <format>
#include <iostream>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>

static void error_printer(int error_code, std::string_view error_source) {
	std::string error_message = std::generic_category().message(error_code);
	std::cerr << std::format("Error in {}: {}\n", error_source, error_message);
	return;
}

WaitStatus wait_readable(int fd, int timeout_ms) {
	pollfd pfd{};
	pfd.fd = fd;
	pfd.events = POLLIN;
	pfd.revents = 0;

	const int result = poll(&pfd, 1, timeout_ms);

	if (result > 0) {
		if (pfd.revents & POLLNVAL) {
			return WaitStatus::Error;
		}

		return WaitStatus::Readable;
	}
	if (result == -1) {
		int error_code = errno;
		if (error_code == EINTR) {
			return WaitStatus::Interrupted;
		}
		error_printer(error_code, "poll");

		return WaitStatus::Error;
	}
	return WaitStatus::Timeout;
}

ReadStatus read_some(int fd, std::string &buffer) {
	char tmp[4096];

	ssize_t bytes_read = read(fd, tmp, sizeof(tmp));
	if (bytes_read == -1) {
		if (errno == EINTR) {
			return read_some(fd, buffer);
		}
		return ReadStatus::Error;
	}
	if (bytes_read == 0) {
		return ReadStatus::Closed;
	}

	buffer.append(tmp, bytes_read);
	return ReadStatus::Data;
}

bool send_all(int fd, std::string_view data) {
	size_t sent = 0;
	while (sent < data.size()) {
		ssize_t sent_now = send(fd, data.data() + sent, data.size() - sent, MSG_NOSIGNAL);
		if (sent_now == -1) {
			if (errno == EINTR) {
				continue;
			}
			return false;
		}
		if (sent_now == 0) {
			return false;
		}
		sent += static_cast<size_t>(sent_now);
	}
	return true;
}

static std::atomic<bool> stop_signal = false;

static void handle_stop_signal(int signo) {
	if (signo == SIGINT || signo == SIGTERM)
		stop_signal.store(true);
}

bool stop_requested() { return stop_signal.load(); }

bool install_signal_handlers() {
	struct sigaction sa{};

	sa.sa_handler = handle_stop_signal;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	if (sigaction(SIGINT, &sa, nullptr) == -1) {
		error_printer(errno, "SIGINT");
		return false;
	}

	if (sigaction(SIGTERM, &sa, nullptr) == -1) {
		error_printer(errno, "SIGTERM");
		return false;
	}

	sa.sa_handler = SIG_IGN;

	if (sigaction(SIGPIPE, &sa, nullptr) == -1) {
		error_printer(errno, "SIGPIPE");
		return false;
	}

	return true;
}

int create_listen_socket(std::uint16_t port) {
	int fd = socket(AF_INET, SOCK_STREAM, 0);

	if (fd == -1) {
		error_printer(errno, "socket");
		return -1;
	}

	int opt = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
		int error_code = errno;
		close(fd);
		error_printer(error_code, "setsockopt");
		return -1;
	}

	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(port);

	if (bind(fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == -1) {
		int error_code = errno;
		close(fd);
		error_printer(error_code, std::format("bind on port {}", port));
		return -1;
	}

	if (listen(fd, SOMAXCONN) == -1) {
		int error_code = errno;
		close(fd);
		error_printer(error_code, "listen");
		return -1;
	}

	return fd;
}