CC      ?= gcc
CFLAGS  ?= -std=c11 -Wall -Wextra -Wpedantic -O2
CPPFLAGS = -Iinclude
LDFLAGS ?=
LDLIBS  ?= -lm

BUILD_DIR := build
BIN       := $(BUILD_DIR)/advection
TEST_BIN  := $(BUILD_DIR)/test_solver
LIB_SRCS  := src/advection.c
APP_SRCS  := src/main.c
LIB_OBJS  := $(LIB_SRCS:src/%.c=$(BUILD_DIR)/%.o)
APP_OBJS  := $(APP_SRCS:src/%.c=$(BUILD_DIR)/%.o)
TEST_OBJS := $(BUILD_DIR)/test_solver.o

.PHONY: all run plot figures test clean distclean

all: $(BIN)

$(BIN): $(LIB_OBJS) $(APP_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

$(TEST_BIN): $(LIB_OBJS) $(TEST_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

$(BUILD_DIR)/%.o: src/%.c include/advection.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/test_solver.o: tests/test_solver.c include/advection.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: $(BIN)
	./$(BIN)

plot: run
	python3 python/plot_results.py

figures: $(BIN) plot
	python3 python/convergence.py
	python3 python/compare_schemes.py
	python3 python/animate.py

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -rf $(BUILD_DIR)

distclean: clean
	rm -rf data results
	rm -f docs/assets/advection_profiles.png \
	      docs/assets/convergence.png \
	      docs/assets/scheme_comparison.png \
	      docs/assets/advection_animation.gif
