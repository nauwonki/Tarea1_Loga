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
TESTS       := $(BUILD_DIR)/test_graph $(BUILD_DIR)/test_fibonacci

.PHONY: all test plots clean

all: $(EXPERIMENTS) $(TESTS)

$(EXPERIMENTS): $(LIB_OBJ) $(BUILD_DIR)/main.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

# Cada test enlaza solo lo que necesita, para poder correrlos aunque falten
# partes por implementar.
$(BUILD_DIR)/test_graph: $(BUILD_DIR)/graph.o $(BUILD_DIR)/test_graph.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD_DIR)/test_fibonacci: $(BUILD_DIR)/graph.o $(BUILD_DIR)/measure.o \
                             $(BUILD_DIR)/fibonacci.o $(BUILD_DIR)/prim_fibonacci.o \
                             $(BUILD_DIR)/test_fibonacci.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(TEST_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Se corre desde la raiz del repositorio porque los tests leen tests/small10.txt.
test: $(TESTS)
	./$(BUILD_DIR)/test_graph
	./$(BUILD_DIR)/test_fibonacci

# Procesa los CSV de results/ y deja los graficos y la tabla en figures/.
plots:
	python3 scripts/plot.py

clean:
	rm -rf $(BUILD_DIR)
