#include "plugins.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
uint32_t ticks;
void _step(float mhz, float clocks) {
	ticks += (uint32_t)((clocks / mhz) * 100);
}
uint8_t _get_via1(uint8_t *regs, uint8_t reg, uint8_t *ptr) {
	if (reg == 0) {
		*ptr = (ticks % (1 << 4)) & (~regs[2]);
		return 0b1111;
	}
	return false;
}
bool _set_via1(uint8_t *regs, uint8_t reg, uint8_t value) {
	if (reg == 0) {
		if (~regs[2] & 0b111) {
			ticks = (ticks & ~regs[2]) | (value & ~regs[2]);
			return true;
		}
	}
	return false;
}
uint8_t _get_via2(uint8_t *regs, uint8_t reg, uint8_t *ptr) {
	if (reg == 0) {
		*ptr = regs[2] & (ticks >> 3);
		return 255;
	} else if (reg == 1) {
		*ptr = regs[3] & (ticks >> 11);
		return 255;
	}
	return false;
}
bool _set_via2(uint8_t *regs, uint8_t reg, uint8_t value) {
	if (reg == 0) {
		ticks = (ticks & (~(((uint32_t)regs[2]) << 3))) | ((uint32_t)value << 3);
	} else if (reg == 1) {
		ticks = (ticks & (~(((uint32_t)regs[3]) << 11))) | ((uint32_t)value << 11);
	}
	return false;
}
PLUGIN_NAME("VIA plugin test");
void main(plugin_t *plugin) {
	ticks = 0;
	plugin->step = &_step;
	plugin->set_via1 = &_set_via1;
	plugin->get_via1 = &_get_via1;
	plugin->set_via2 = &_set_via2;
	plugin->get_via2 = &_get_via2;
}