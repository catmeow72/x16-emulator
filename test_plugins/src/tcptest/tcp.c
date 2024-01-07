#include "plugins.h"
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <poll.h>
typedef struct TCPRegs {
	uint8_t ip_addr[4];
	uint8_t ip_addr_idx_out;
	uint8_t ip_addr_idx_in;
	uint16_t port;
	uint8_t port_idx_in;
	bool connected;
	bool waiting;
	uint8_t in_buf[64];
	uint8_t out_buf[64];
	size_t in_size;
	size_t out_size;
	int sock;
} TCPRegs;
static TCPRegs *regs;
typedef enum IOReg {
	IOIPAddr = 0,
	IOTCPPort = 1,
	IOConnected = 2,
	IOConnect = 2,
	IODisconnect = 2,
	IOData = 3,
	IOInEmpty = 4,
	IOClearIn = 4,
	IOInFull = 5,
	IOOutEmpty = 6,
	IOFlushOut = 6,
	IOOutFull = 7,
	IOReset = 7,
	IOMax = 8
} IOReg;
float clock_value = 0;
uint8_t value = 0;
void _step(float mhz, float clocks);
uint8_t _get_iobase() {
	return 4;
}
uint8_t _io_read(uint8_t idx) {
	TCPRegs *reg = &regs[idx / 8];
	switch ((IOReg)idx % IOMax) {
		case IOIPAddr: {
			uint8_t output = 0;
			output = reg->ip_addr[reg->ip_addr_idx_in++];
			reg->ip_addr_idx_in %= 4;
			return output;
		}
		case IOTCPPort: {
			if (reg->port_idx_in++ == 0) {
				return reg->port & 0xFF;
			} else {
				reg->port_idx_in = 0;
				return reg->port >> 8;
			}
		}
		case IOConnected: {
			return reg->connected;
		}
		case IOData: {
			if (reg->connected) {
				if (reg->in_size > 0) {
					uint8_t output;
					output = reg->in_buf[0];
					memmove(reg->in_buf, reg->in_buf + 1, reg->in_size--);
					return output;
				} else {
					return 0;
				}
			}
		}
		case IOInEmpty: {
			return reg->connected && reg->in_size == 0;
		}
		case IOInFull: {
			return reg->connected && reg->in_size >= 64;
		}
		case IOOutEmpty: {
			return reg->connected && reg->out_size == 0;
		}
		case IOOutFull: {
			return reg->connected && reg->out_size >= 64;
		}
	}
	return 0;
}

void _io_write(uint8_t data, uint8_t idx) {
	TCPRegs *reg = &regs[idx / IOMax];
	switch ((IOReg)idx % IOMax) {
		case IOIPAddr: {
			reg->ip_addr[reg->ip_addr_idx_out++] = data;
			reg->ip_addr_idx_out %= 4;
			printf("IP Address: %u.%u.%u.%u\n", reg->ip_addr[0], reg->ip_addr[1], reg->ip_addr[2], reg->ip_addr[3]);
		} break;
		case IOTCPPort: {
			reg->port <<= 8;
			reg->port |= data;
			printf("Port: %u\n", reg->port);
		} break;
		case IOConnected: {
			if (data) {
				printf("Connecting to %u.%u.%u.%u:%u... ", reg->ip_addr[0], reg->ip_addr[1], reg->ip_addr[2], reg->ip_addr[3], reg->port);
				reg->sock = socket(AF_INET, SOCK_STREAM, 0);
				struct sockaddr_in serv_addr;
				if (reg->sock >= 0) {
					int flags = fcntl(reg->sock, F_GETFL, 0);
					fcntl(reg->sock, F_SETFL, flags | O_NONBLOCK);
					serv_addr.sin_family = AF_INET;
					serv_addr.sin_port = htons(reg->port);
					struct in_addr addr;
					memcpy((void*)&addr, reg->ip_addr, sizeof(struct in_addr));
					serv_addr.sin_addr = addr;
					if (connect(reg->sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) != -1) {
						reg->connected = true;
						printf("Success!");
					} else {
						if (errno == EINPROGRESS) {
							reg->waiting = true;
							printf(" ... waiting ... ");
						} else {
							printf("Error connecting: %s", strerror(errno));
						}
					}
				} else {
					printf("Socket creation error.");
				}
				printf("\n");
			} else {
				printf("Disconnecting...\n");
				close(reg->sock);
			}
		} break;
		case IOData: {
			if (reg->connected) {
				if (reg->out_size < 64) {
					reg->out_buf[reg->out_size++] = data;
				}
			}
		} break;
		case IOClearIn: {
			reg->in_size = 0;
		} break;
		case IOFlushOut: {
			reg->out_size = 0;
		} break;
		case IOReset: {
			close(reg->sock);
			reg->in_size = 0;
			reg->out_size = 0;
			reg->ip_addr[0] = 0;
			reg->ip_addr[1] = 0;
			reg->ip_addr[2] = 0;
			reg->ip_addr[3] = 0;
			reg->ip_addr_idx_in = 0;
			reg->ip_addr_idx_out = 0;
			reg->port_idx_in = 0;
			reg->port = 0;
			printf("Reset successful.\n");
		} break;
	}
}
PLUGIN_NAME("IO plugin test");
clinkage void main(plugin_t *plugin) {
	regs = (TCPRegs*)malloc(sizeof(TCPRegs) * 2);
	memset(regs, 0, sizeof(TCPRegs) * 2);
	plugin->step = &_step;
	plugin->get_io_base = &_get_iobase;
	plugin->get_io = &_io_read;
	plugin->set_io = &_io_write;
}
void _step(float mhz, float clocks) {
	for (size_t i = 0; i < 2; i++) {
		TCPRegs *reg = &regs[i];
		if (reg->waiting) {
			struct pollfd poll_fd;
			poll_fd.fd = reg->sock;
			poll_fd.events = POLLOUT;
			if (poll(&poll_fd, 1, 0) != -1) {
				reg->connected = true;
				reg->waiting = false;
				printf("Connected!\n");
			}
		}
		if (reg->connected) {
			if (reg->out_size > 0) {
				send(reg->sock, &reg->out_buf[0], 1, 0);
				reg->out_size--;
				if (reg->out_size > 0) {
					memcpy(reg->out_buf, reg->out_buf + 1, reg->out_size);
				}
			}
			if (reg->in_size < 64) {
				uint8_t buf[2];
				if (read(reg->sock, &buf, 1) >= 1) {
					reg->in_buf[reg->in_size++] = buf[0];
				}
			}
		}
	}
}