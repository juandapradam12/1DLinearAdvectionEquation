CC      ?= gcc
CFLAGS  ?= -std=c11 -Wall -Wextra -Wpedantic -O2
CPPFLAGS = -Iinclude
LDFLAGS ?=
LDLIBS  ?= -lm

BUILD_DIR := build
BIN       := $(BUILD_DIR)/advection
SRCS      := src/advection.c src/main.c
OBJS      := $(SRCS:src/%.c=$(BUILD_DIR)/%.o)

.PHONY: all run plot clean distclean

all: $(BIN)

$(BIN): $(OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS) $(LDLIBS)

$(BUILD_DIR)/%.o: src/%.c include/advection.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: $(BIN)
	./$(BIN)

plot: run
	python3 python/plot_results.py

clean:
	rm -rf $(BUILD_DIR)

distclean: clean
	rm -rf data results
	rm -f docs/assets/advection_profiles.png
