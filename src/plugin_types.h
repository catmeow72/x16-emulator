#pragma once
#include <stdint.h>
#include <stdbool.h>
typedef struct _plugin_t plugin_t;
typedef void (*plugin_initializer_t)(plugin_t *plugin);
typedef const char *(*plugin_name_getter_t)();
typedef struct _plugin_t {
	void *handle;
	plugin_name_getter_t get_name;
	uint8_t (*get_io_base)();
	uint8_t (*get_io)(uint8_t idx);
	void (*set_io)(uint8_t data, uint8_t idx);
	uint8_t (*get_via1)(uint8_t *regs, uint8_t reg, uint8_t *ptr);
	bool (*set_via1)(uint8_t *regs, uint8_t reg, uint8_t value);
	uint8_t (*get_via2)(uint8_t *regs, uint8_t reg, uint8_t *ptr);
	bool (*set_via2)(uint8_t *regs, uint8_t reg, uint8_t value);
	void (*step)(float mhz, float cycles);
	bool (*irq)();
	void (*deinit)();
	plugin_initializer_t init;
} plugin_t;