#pragma once
#include "timing.h"
#include "plugin_types.h"
extern const char *plugin_suffix;
extern plugin_t **io_plugins;
extern plugin_t **plugins;
extern size_t plugin_count;
extern size_t plugin_capacity;
extern const char *plugin_system_error;
bool plugin_system_init();
bool plugin_load(const char *filename);
bool plugin_add_builtin(plugin_t *plugin);
void plugin_init(plugin_t *plugin);
void plugin_system_unload();
uint8_t plugin_get_io(uint8_t iobase, uint8_t idx);
void plugin_set_io(uint8_t iobase, uint8_t data, uint8_t idx);
void plugin_step(float mhz, float cycles);
uint8_t plugin_get_via1(uint8_t *regs, uint8_t reg, uint8_t *ptr);
bool plugin_set_via1(uint8_t *regs, uint8_t reg, uint8_t value);
uint8_t plugin_get_via2(uint8_t *regs, uint8_t reg, uint8_t *ptr);
bool plugin_set_via2(uint8_t *regs, uint8_t reg, uint8_t value);
bool plugin_irq();