BUILD ?= debug

CC  := cc
CSTD := -std=c11
WARN := -Wall -Wextra -Werror
INCLUDE := -Isrc -Ithird_party

SAN             := -fsanitize=undefined,address
CFLAGS_debug    := -O0 -g $(SAN)
LDFLAGS_debug   := $(SAN)
CFLAGS_release  := -O2 -DNDEBUG
LDFLAGS_release :=

CFLAGS := $(CSTD) $(WARN) $(INCLUDE) -MMD -MP $(CFLAGS_$(BUILD))
LDFLAGS := $(LDFLAGS_$(BUILD))

OBJDIR := build/$(BUILD)
BINDIR := $(OBJDIR)/bin


SRCS := $(wildcard src/*.c)
OBJS := $(patsubst src/%.c,$(OBJDIR)/src/%.o,$(SRCS))

TEST_SRCS := $(wildcard tests/*.c)
TEST_OBJS := $(patsubst tests/%.c,$(OBJDIR)/tests/%.o,$(TEST_SRCS))
TEST_BINS := $(patsubst tests/%.c,$(BINDIR)/%,$(TEST_SRCS))

all: $(TEST_BINS)

$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BINDIR)/%: $(OBJDIR)/tests/%.o $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) $^ -o $@

test: $(TEST_BINS)
	@for t in $(TEST_BINS); do echo "== $$t"; $$t || exit 1; done

test-release:
	@$(MAKE) --no-print-directory BUILD=release test

clean:
	rm -rf build

-include $(OBJS:.o=.d) $(TEST_OBJS:.o=.d)

