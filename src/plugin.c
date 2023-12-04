#include "plugin_types.h"
#include <SDL.h>
#include <stdio.h>
#include <string.h>
const char *plugin_suffix = 
#ifdef _WIN32
".dll"
#elif defined(__APPLE__)
".bundle"
#elif defined(__unix__)
".so"
#else
""
#endif
;
plugin_t **io_plugins = NULL;
plugin_t **plugins = NULL;
size_t plugin_count;
size_t plugin_capacity;
char *plugin_system_error;
void plugin_set_error(const char *fmt, ...) {
	va_list args, args_backup;
	char *buf;
	if (plugin_system_error != NULL) {
		free(plugin_system_error);
	}
	va_start(args, fmt);
	va_copy(args_backup, args);
	size_t buflen = vsnprintf(NULL, 0, fmt, args) + 1;
	va_end(args);
	buf = (char*)malloc(buflen);
	if (buf == NULL) {
		vprintf(fmt, args_backup);
		va_end(args_backup);
		return;
	}
	buflen = vsnprintf(buf, buflen, fmt, args_backup);
	plugin_system_error = buf;
	printf("%s\n", buf);
	va_end(args_backup);
}
const char *default_name_getter() {
	return "(untitled plugin)";
}
void plugin_system_unload();
bool plugin_system_init() {
	if (plugins == NULL) {
		plugin_capacity = 4;
		plugin_count = 0;
		plugins = (plugin_t**)malloc((sizeof(plugin_t*) * plugin_capacity));
		if (plugins == NULL) {
			plugin_set_error("Error allocating memory for plugins");
			plugin_system_unload();
			return false;
		}
	}
	if (io_plugins == NULL) {
		io_plugins = (plugin_t**)malloc(sizeof(plugin_t*)*5);
		if (io_plugins == NULL) {
			plugin_set_error("Error allocating space for IO plugins");
			plugin_system_unload();
			return false;
		}
		memset(io_plugins, 0, sizeof(plugin_t*)*5);
	}
	return true;
}
void plugin_init(plugin_t *plugin) {
	(*(plugin->init))(plugin);
	if (plugin->get_io_base != NULL) {
		int iobase = (*(plugin->get_io_base))();
		printf("Plugin iobase: %u - 3 = %u\n", iobase, iobase - 3);
		io_plugins[iobase - 3] = plugin;
	}
}
bool plugin_add(plugin_t *plugin, bool builtin) {
	if (builtin) {
		printf("Adding built-in plugin: ");
	} else {
		printf("Plugin name: ");
	}
	if (plugin->get_name == NULL) {
		plugin->get_name = &default_name_getter;
	}
	printf("%s\n", (*(plugin->get_name))());
	plugin_init(plugin);
	if (++plugin_count > plugin_capacity) {
		plugin_capacity *= 2;
		plugins = (plugin_t**)realloc(plugins, sizeof(plugin_t*) * plugin_capacity);
		if (plugins == NULL) {
			plugin_set_error("Error allocating memory for plugins");
			if (plugin->handle == NULL) {
				SDL_UnloadObject(plugin->handle);
			}
			plugin_system_unload();
			return false;
		}
	}
	plugins[plugin_count - 1] = plugin;
	return true;
}
bool plugin_load(const char *filename) {
	void *handle = NULL;
	printf("Loading plugin %s...\n", filename);
	handle = SDL_LoadObject(filename);
	if (handle == NULL) {
		plugin_set_error("Error loading plugin file: %s", SDL_GetError());
		plugin_system_unload();
		return false;
	}
	void *initializer = SDL_LoadFunction(handle, "plugin_init");
	if (initializer == NULL) {
		plugin_set_error("Error loading plugin initializer: %s", SDL_GetError());
		SDL_UnloadObject(handle);
		plugin_system_unload();
		return false;
	}
	void *name_getter = SDL_LoadFunction(handle, "get_name");
	if (name_getter == NULL) {
		name_getter = (void*)&default_name_getter;
	}
	plugin_t *plugin = (plugin_t*)malloc(sizeof(plugin_t));
	if (plugin == NULL) {
		plugin_set_error("Error allocating memory for plugin");
		SDL_UnloadObject(handle);
		plugin_system_unload();
		return false;
	}
	memset(plugin, 0, sizeof(plugin_t));
	plugin->handle = handle;
	plugin->get_name = (plugin_name_getter_t)name_getter;
	plugin->init = (plugin_initializer_t)initializer;
	return plugin_add(plugin, false);
}
bool plugin_add_builtin(plugin_t *plugin) {
	return plugin_add(plugin, true);
}
void plugin_system_unload() {
	printf("Unloading plugins...\n");
	if (io_plugins != NULL) {
		free(io_plugins);
		io_plugins = NULL;
	}
	if (plugins != NULL) {
		for (size_t i = 0; i < plugin_count; i++) {
			if (plugins[i] != NULL) {
				if (plugins[i]->deinit != NULL) (*(plugins[i]->deinit))();
				if (plugins[i]->handle != NULL) {
					SDL_UnloadObject(plugins[i]->handle);
					plugins[i]->handle = NULL;
				}
				free(plugins[i]);
			}
		}
		free(plugins);
		plugins = NULL;
	}
}
uint8_t plugin_get_io(uint8_t iobase, uint8_t idx) {
	if (io_plugins == NULL) return 0;
	uint8_t pluginidx = iobase - 3;
	if (io_plugins[pluginidx] == NULL || io_plugins[pluginidx]->get_io == NULL) {
		return 0x9f; // open bus read
	} else {
		return (*io_plugins[pluginidx]->get_io)(idx);
	}
}
void plugin_set_io(uint8_t iobase, uint8_t data, uint8_t idx) {
	if (io_plugins == NULL) return;
	uint8_t pluginidx = iobase - 3;
	if (io_plugins[pluginidx] == NULL || io_plugins[pluginidx]->set_io == NULL) {
		return;
	} else {
		(*(io_plugins[pluginidx]->set_io))(data, idx);
	}
}
void plugin_step(float mhz, float cycles) {
	if (plugins == NULL) return;
	for (size_t i = 0; i < plugin_count; i++) {
		if (plugins[i]->step != NULL) {
			(*(plugins[i]->step))(mhz, cycles);
		}
	}
}
uint8_t plugin_get_via1(uint8_t *regs, uint8_t reg, uint8_t *ptr) {
	if (plugins == NULL) return false;
	uint8_t output = 0;
	for (size_t i = 0; i < plugin_count; i++) {
		if (plugins[i]->get_via1 != NULL) {
			output |= (*(plugins[i]->get_via1))(regs, reg, ptr);
		}
	}
	return output;
}
bool plugin_set_via1(uint8_t *regs, uint8_t reg, uint8_t value) {
	if (plugins == NULL) return false;
	bool success = false;
	for (size_t i = 0; i < plugin_count; i++) {
		if (plugins[i]->set_via1 != NULL) {
			if ((*(plugins[i]->set_via1))(regs, reg, value)) {
				success = true;
			}
		}
	}
	return success;
}
uint8_t plugin_get_via2(uint8_t *regs, uint8_t reg, uint8_t *ptr) {
	if (plugins == NULL) return false;
	uint8_t output = 0;
	for (size_t i = 0; i < plugin_count; i++) {
		if (plugins[i]->get_via2 != NULL) {
			output |= (*(plugins[i]->get_via2))(regs, reg, ptr);
		}
	}
	return output;
}
bool plugin_set_via2(uint8_t *regs, uint8_t reg, uint8_t value) {
	if (plugins == NULL) return false;
	bool success = false;
	for (size_t i = 0; i < plugin_count; i++) {
		if (plugins[i]->set_via2 != NULL) {
			if ((*(plugins[i]->set_via2))(regs, reg, value)) {
				success = true;
			}
		}
	}
	return success;
}
bool plugin_irq() {
	if (plugins == NULL) return false;
	for (size_t i = 0; i < plugin_count; i++) {
		if (plugins[i]->irq != NULL) {
			if ((*(plugins[i]->irq))()) {
				return true;
			}
		}
	}
	return false;
}