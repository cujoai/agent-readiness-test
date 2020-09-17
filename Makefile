# Copyright (c) 2020 CUJO LLC. All rights reserved.

################################################################################
# Overrideable by the caller

OUT_DIR ?= out
PKG_CONFIG ?= pkg-config

# Flags split by dependency, for convenient overriding

# AGENT_READINESS_TEST_CFLAGS_libuv := $(shell $(PKG_CONFIG) --cflags libuv)
# AGENT_READINESS_TEST_LDFLAGS_libuv := $(shell $(PKG_CONFIG) --libs-only-L libuv)
# AGENT_READINESS_TEST_LDLIBS_libuv := $(shell $(PKG_CONFIG) --libs-only-l libuv)

AGENT_READINESS_TEST_LDLIBS_lua := -ldl -llua -lm

# Combined flags

AGENT_READINESS_TEST_CFLAGS := \
	$(AGENT_READINESS_TEST_CFLAGS_lua)
	# $(AGENT_READINESS_TEST_CFLAGS_libuv)
AGENT_READINESS_TEST_LDFLAGS := \
	$(AGENT_READINESS_TEST_LDFLAGS_lua)
	# $(AGENT_READINESS_TEST_LDFLAGS_libuv)
AGENT_READINESS_TEST_LDLIBS := \
	$(AGENT_READINESS_TEST_LDLIBS_lua)
	# $(AGENT_READINESS_TEST_LDLIBS_libuv)

# Add the AGENT_READINESS_TEST_ options on top of any standard ones that have
# been passed in.  This way the caller can specify e.g. warning and
# optimization flags without overriding flags that are needed for our
# dependencies to work. If the caller wants to override the dependency flags
# (e.g. because they don't have pkg-config), they can set the
# AGENT_READINESS_TEST_ options.
#
# The AGENT_READINESS_TEST_ flags have to be first so that things like -I and
# -L are processed in the correct order: We want the ones for our dependencies
# to override any global ones, so that we pick up the right versions (e.g. our
# Lua and not the system's default one).
override CFLAGS := $(AGENT_READINESS_TEST_CFLAGS) $(CFLAGS)
override LDFLAGS := $(AGENT_READINESS_TEST_LDFLAGS) $(LDFLAGS)
override LDLIBS := $(AGENT_READINESS_TEST_LDLIBS) $(LDLIBS)

# End overrideable by the caller, anything below is internal
################################################################################

SOURCES := $(wildcard *.c)
MAIN_SOURCES := $(filter-out test.c,$(SOURCES))

# Don't delete intermediate files, like our .d dependency files.
.SECONDARY:

# Don't use make's built-in rules.
.SUFFIXES:

.PHONY: main
all: main
main: $(OUT_DIR)/agent-readiness-test

# Build executables from objects.

$(OUT_DIR)/agent-readiness-test: $(patsubst %.c,$(OUT_DIR)/%.o,$(MAIN_SOURCES))
	$(CC) -o "$@" $^ $(LDFLAGS) $(LDLIBS)

# Build objects from C code.
#
# Use -std=c99 instead of anything newer because we need to support platforms
# with GCC 4.5 (or older), which don't have even -std=c1x.
#
# Define _POSIX_C_SOURCE to the latest version at the time of GCC 4.5 as well,
# which is 200809L. Almost all of the code depends on it anyway.
#
# Depend on the .d (dependency) files also, forcing a rebuild if they are
# deleted (in combination with the %.d rule below).
#
# Unusual flags explained:
#   -MD: Generate dependency output together with compilation.
#   -MP: Generate blank targets for each prerequisite to avoid errors when they
#        are deleted.
#   -MQ x: Specify the path for which the dependency rule is for.
#   -MF x: Specify the path where the dependencies should be written.

STDFLAGS := -std=c99 -D_POSIX_C_SOURCE=200809L

$(OUT_DIR)/%.o: %.c $(OUT_DIR)/%.d Makefile | $(OUT_DIR)
	$(CC) -o "$@" -c $< -MD -MP -MQ "$@" -MF "$(OUT_DIR)"/$*.d $(STDFLAGS) $(CPPFLAGS) $(CFLAGS)

# Touch missing dependency files, so that they are treated as out of date if
# they are deleted.
$(OUT_DIR)/%.d: | $(OUT_DIR)
	touch "$@"

$(OUT_DIR):
	mkdir -p "$@"

# Load the dependency files, so that changes to header files trigger the
# expected rebuilds. (Use "-include" to ignore any missing ones.)
-include $(patsubst %.c,$(OUT_DIR)/%.d,$(MAIN_SOURCES))
