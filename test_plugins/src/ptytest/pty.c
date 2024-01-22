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
#include <pty.h>
typedef struct TCPRegs {
	uint8_t in_buf[255];
	uint8_t out_buf[255];
	size_t in_size;
	size_t out_size;
	int masterfd;
	int slavefd;
	char name[256];
} TCPRegs;
static TCPRegs *regs;
typedef enum IOReg {
	IOData = 0,
	IOInCount = 1,
	IOFlushIn = 1,
	IOOutCount = 2,
	IOFlushOut = 2,
	IOReset = 3,
	IOMax = 4
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
		case IOData: {
			if (reg->in_size > 0) {
				uint8_t output;
				output = reg->in_buf[0];
				memmove(reg->in_buf, reg->in_buf + 1, reg->in_size--);
				return output;
			} else {
				return 0;
			}
		}
		case IOInCount: {
			return reg->in_size;
		}
		case IOOutCount: {
			return reg->out_size;
		}
	}
	return 0;
}

void _io_write(uint8_t data, uint8_t idx) {
	TCPRegs *reg = &regs[idx / IOMax];
	switch ((IOReg)idx % IOMax) {
		case IOData: {
			if (reg->out_size < 255) {
				reg->out_buf[reg->out_size++] = data;
			}
		} break;
		case IOFlushIn: {
			reg->in_size = 0;
		} break;
		case IOFlushOut: {
			reg->out_size = 0;
		} break;
		case IOReset: {
			reg->in_size = 0;
			reg->out_size = 0;
		} break;
	}
}
void _deinit() {
	for (size_t i = 0; i < 4; i++) {
		close(regs[i].masterfd);
		close(regs[i].slavefd);
	}
	free(regs);
}
PLUGIN_NAME("IO plugin test");
clinkage void main(plugin_t *plugin) {
	regs = (TCPRegs*)malloc(sizeof(TCPRegs) * 4);
	memset(regs, 0, sizeof(TCPRegs) * 4);
	plugin->deinit = &_deinit;
	plugin->step = &_step;
	plugin->get_io_base = &_get_iobase;
	plugin->get_io = &_io_read;
	plugin->set_io = &_io_write;
	for (size_t i = 0; i < 4; i++) {
		int pid = openpty(&regs[i].masterfd, &regs[i].slavefd, regs[i].name, NULL, NULL);
		int flags = fcntl(regs[i].masterfd, F_GETFL, 0);
		fcntl(regs[i].masterfd, F_SETFL, flags | O_NONBLOCK);
		printf("PTY #%u: %s\n", i, regs[i].name);
	}
}
void _step(float mhz, float clocks) {
	for (size_t i = 0; i < 2; i++) {
		TCPRegs *reg = &regs[i];
		if (reg->out_size > 0) {
			write(reg->masterfd, &reg->out_buf[0], 1);
			reg->out_size--;
			if (reg->out_size > 0) {
				memcpy(reg->out_buf, reg->out_buf + 1, reg->out_size);
			}
		}
		if (reg->in_size < 255) {
			uint8_t buf[2];
			if (read(reg->masterfd, &buf, 1) >= 1) {
				reg->in_buf[reg->in_size++] = buf[0];
			}
		}
	}
}
