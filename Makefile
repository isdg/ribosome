BUILD ?= debug

CC  := cc
CSTD := -std=c11
WARN := -Warn -Wextra -Werror
INCLUDE := -Isrc -Ithird_party

SAN := -fsanitize=undefined,address
CFLAGS_debug := -02 -DNDEBUG
LDFLAGS_release := 

CFLAGS := $(CSTD) $(WARN) $(INCLUDE) -MMD -MP $(CFLAGS_$(BUILD))
LDFLAGS := $(LDFLAGS_$(BUILD))

OBJDIR := build/$(BUILD)
BINDIR := $(OBJDIR)/bin

