MAKEFILE_DIR = $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
define ADD_PLUGIN
PLUGIN_IDS        += $(1)
$(1)_SRC_DIR       = $$(MAKEFILE_DIR)/src/$(1)
$(1)_C_FILES       = $$(wildcard $$($(1)_SRC_DIR)/*.c)
$(1)_CXX_FILES     = $$(wildcard $$($(1)_SRC_DIR)/*.cpp)
$(1)_OBJECTS       = $$($(1)_C_FILES:.c=.o) $$($(1)_CXX_FILES:.cpp=.o)
$(1)_PLUGIN_NAME   = $(1)
$(1)_COMMONFLAGS  := $$(COMMONFLAGS)
$(1)_CFLAGS       := $$(CFLAGS) $$($(1)_COMMONFLAGS)
$(1)_CXXFLAGS     := $$(CXXFLAGS) $$($(1)_COMMONFLAGS)
$(1)_LDFLAGS      := $$(LDFLAGS)
$(1)_FILENAME     := $$(MAKEFILE_DIR)/$$($(1)_PLUGIN_NAME)$$(SUFFIX)
$(1)_OBJDIR       := $$(MAKEFILE_DIR)/$$(OUTDIR)/$$($(1)_PLUGIN_NAME)
$(1)_CLEAN_TARGET := $(1)_clean
ALL_PLUGINS       += $$($(1)_FILENAME)
OUT_DIRS          += $$($(1)_OBJDIR)
CLEAN_TARGETS     += $$($(1)_CLEAN_TARGET)
PLUGIN_DISPLAYERS += $(1)_disp
$(1)_OBJECTS      := $$(addprefix $$($(1)_OBJDIR)/,$$(notdir $$($(1)_OBJECTS)))
.PHONY: $(1)_disp
$(1)_disp:
	@echo "Plugin                $$($(1)_PLUGIN_NAME)"
	@echo "    Plugin ID         $(1)"
	@echo "    C flags           $$($(1)_CFLAGS)"
	@echo "    C++ flags         $$($(1)_CXXFLAGS)"
	@echo "    Linker flags      $$($(1)_LDFLAGS)"
	@echo "    Source directory  $$($(1)_SRC_DIR)"
	@echo "    Object directory  $$($(1)_OBJDIR)"
	@echo "    Source files      $$($(1)_C_FILES) $$($(1)_CXX_FILES)"
	@echo "    Object files      $$($(1)_OBJECTS)"
	@echo "    Filename          $$($(1)_FILENAME)"
	@echo "    Clean target      $$($(1)_CLEAN_TARGET)"
$$($(1)_OBJDIR)/%.o: $$($(1)_SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $$< -o $$@
$$($(1)_OBJDIR)/%.o: $$($(1)_SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $$< -o $$@
$$($(1)_OBJDIR):
	mkdir -p $$($(1)_OBJDIR)
$$($(1)_FILENAME): $$($(1)_OBJDIR) $$($(1)_OBJECTS)
	$(LD) $(LDFLAGS) $$($(1)_OBJECTS) -o $$($(1)_FILENAME)
$$($(1)_CLEAN_TARGET):
	rm -f $$($(1)_FILENAME)
	rm -rf $$($(1)_OBJDIR)
endef
PLUGIN_DISPLAYERS :=
PLUGINS           :=
CLEAN_TARGETS     :=
OUT_DIRS          :=
PLUGIN_IDS        :=
COMMONFLAGS       := -fpic -shared -I$(MAKEFILE_DIR) -g
CFLAGS            := $(COMMONFLAGS) -std=gnu11
CXXFLAGS          := $(COMMONFLAGS) -std=gnu++11
LDFLAGS           := -fpic -shared
SUFFIX            := 
OUTDIR            := obj
PLATFORM          := detect
# Detect OS
ifeq ($(PLATFORM),detect)
	ifeq ($(OS),Windows_NT)
		PLATFORM = win32
	else
		UNAME_S := $(shell uname -s)
		ifeq ($(UNAME_S),Linux)
			PLATFORM = linux
		endif
		ifeq ($(UNAME_S),Darwin)
			PLATFORM = macos
		endif
	endif
endif
# Make decisions based on OS
ifeq ($(PLATFORM),win32)
	SUFFIX = .dll
endif
ifeq ($(PLATFORM),linux)
	SUFFIX = .so
endif
ifeq ($(PLATFORM),macos)
	SUFFIX = .bundle
	LDFLAGS += -bundle
endif
.PHONY: clean all plugins build_plugins
all: build_plugins
$(eval $(call ADD_PLUGIN,iotest))
$(eval $(call ADD_PLUGIN,viatest))
$(eval $(call ADD_PLUGIN,tcptest))
$(eval $(call ADD_PLUGIN,ptytest))
build_plugins: $(ALL_PLUGINS)
clean: $(CLEAN_TARGETS)
plugins: $(PLUGIN_DISPLAYERS)
