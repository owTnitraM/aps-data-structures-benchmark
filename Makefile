CC     = gcc
CFLAGS = -O0 -Wall -Wextra -g -MMD -MP -Isrc
BUILD  = build
TARGET = $(BUILD)/prog.exe

SRC = src/main.c $(wildcard src/Timestamp/*.c) $(wildcard src/DataTypes/*/*.c)
OBJ = $(SRC:%.c=$(BUILD)/%.o)

# Benchmark: tests/bench.c + os .c de src/ (sem src/main.c).
BENCH_SRC = tests/bench.c $(wildcard src/Timestamp/*.c) $(wildcard src/DataTypes/*/*.c)
BENCH_OBJ = $(BENCH_SRC:%.c=$(BUILD)/%.o)
BENCH     = $(BUILD)/bench.exe

# Comandos de shell compativeis com Linux e Windows.
ifeq ($(OS),Windows_NT)
MKDIR = if not exist "$(subst /,\,$(1))" mkdir "$(subst /,\,$(1))"
RMDIR = if exist "$(subst /,\,$(1))" rmdir /s /q "$(subst /,\,$(1))"
RUN   = $(subst /,\,$(1))
else
MKDIR = mkdir -p "$(1)"
RMDIR = rm -rf "$(1)"
RUN   = ./$(1)
endif

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@

$(BENCH): $(BENCH_OBJ)
	$(CC) $(BENCH_OBJ) -o $@

$(BUILD)/%.o: %.c
	@$(call MKDIR,$(@D))
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	@$(call RUN,$(TARGET))

# make bench ARGS="quick"
bench: $(BENCH)
	@$(call RUN,$(BENCH)) $(ARGS)

clean:
	@$(call RMDIR,$(BUILD))

-include $(OBJ:.o=.d) $(BENCH_OBJ:.o=.d)

.PHONY: run bench clean
