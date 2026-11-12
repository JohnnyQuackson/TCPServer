#include "server.h"
#include <charconv>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

struct Config {
	std::uint16_t port = 8080;
	int threads = 8;
};

int get_number(const char *begin) {
	int value = -1;

	const char *end = begin + std::strlen(begin);

	auto [ptr, ec] = std::from_chars(begin, end, value);

	if (ec != std::errc{} || ptr != end) {
		return -1;
	}
	return value;
}

int main(int argc, char *argv[]) {

	Config cfg;
	int i = 1;
	while (i < argc) {
		std::string s(argv[i]);

		if (s == "--port") {
			if (i + 1 < argc) {
				int value = get_number(argv[i + 1]);
				++i;
				if (value >= 1 && value <= 65535) {
					cfg.port = static_cast<std::uint16_t>(value);
				} else {
					std::cerr << "1 <= Port <= 65535!" << std::endl;
					return 2;
				}
			}
		} else if (s == "--threads") {
			if (i + 1 < argc) {
				int value = get_number(argv[i + 1]);
				++i;
				if (value > 0) {
					cfg.threads = value;
				} else {
					std::cerr << "Threads > 0!" << std::endl;
					return 2;
				}
			}
		} else {
			std::cerr << "Unknown parameter: " << s << std::endl;
			return 2;
		}
		++i;
	}

	if (!install_signal_handlers())
		return 1;

	std::cout << "Config is set. Port = " << cfg.port << " Threads = " << cfg.threads << std::endl;

	int fd = create_listen_socket(cfg.port);
	if (fd == -1)
		return 1;

	int client_fd = accept(fd, nullptr, nullptr);

	if (client_fd == -1) {
		close(fd);
		return 1;
	}
	std::string buffer;

	while (true) {

		WaitStatus wait_status = wait_readable(client_fd, 5000);
		if (wait_status == WaitStatus::Readable) {
			ReadStatus read_status = read_some(client_fd, buffer);
			if (read_status == ReadStatus::Data) {
				bool sent_status = send_all(client_fd, buffer);
				if (sent_status == false) {
					int error_code = errno;
					std::string error_message = strerror(error_code);
					std::cerr << "send_all failed: " << error_message << std::endl;
				}
				buffer.clear();
			}
			if (read_status == ReadStatus::Error || read_status == ReadStatus::Closed) {
				break;
			}
		}
		if (wait_status == WaitStatus::Interrupted) {
			if (stop_requested()) {
				break;
			}
		}
		if (wait_status == WaitStatus::Error || wait_status == WaitStatus::Timeout) {
			break;
		}
	}
	close(client_fd);
	close(fd);

	std::cout << "\nServer stopped correctly" << std::endl;
	return 0;
}