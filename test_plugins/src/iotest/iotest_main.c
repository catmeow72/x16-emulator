#include "plugins.h"
#include <stdint.h>
#include <stddef.h>
float clock_value = 0;
uint8_t value = 0;
void _step(float mhz, float clocks);
uint8_t _get_iobase() {
	return 4;
}
uint8_t _io_read(uint8_t idx) {
	return value;
}
void _io_write(uint8_t data, uint8_t idx) {
	value = data;
}
PLUGIN_NAME("IO plugin test");
clinkage void main(plugin_t *plugin) {
	plugin->step = &_step;
	plugin->get_io_base = &_get_iobase;
	plugin->get_io = &_io_read;
	plugin->set_io = &_io_write;
}
void _step(float mhz, float clocks) {
	clock_value += clocks / mhz;
	if (((size_t)clock_value % 100) == 0) {
		value++;
		clock_value -= 100.0f;
	}
}