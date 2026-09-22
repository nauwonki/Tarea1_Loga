CC      ?= gcc
CFLAGS  ?= -std=c11 -O2 -Wall -Wextra -pedantic
LDLIBS  ?= -lm

SRC_DIR   := src
TEST_DIR  := tests
BUILD_DIR := build

# main.c se enlaza aparte porque define main(), igual que los tests.
LIB_SRC := $(filter-out $(SRC_DIR)/main.c,$(wildcard $(SRC_DIR)/*.c))
LIB_OBJ := $(LIB_SRC:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

EXPERIMENTS := $(BUILD_DIR)/tarea1
TESTS       := $(BUILD_DIR)/test_graph

.PHONY: all test clean

all: $(EXPERIMENTS) $(TESTS)

$(EXPERIMENTS): $(LIB_OBJ) $(BUILD_DIR)/main.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

# Los tests solo dependen del grafo, para poder correrlos antes de que las
# colas de prioridad esten implementadas.
$(TESTS): $(BUILD_DIR)/graph.o $(BUILD_DIR)/test_graph.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(TEST_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Se corre desde la raiz del repositorio porque el test lee tests/small10.txt.
test: $(TESTS)
	./$(TESTS)

clean:
	rm -rf $(BUILD_DIR)
