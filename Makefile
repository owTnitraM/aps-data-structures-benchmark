CC     = gcc
CFLAGS = -O0 -Wall -Wextra -g -MMD -MP -Isrc
BUILD  = build
TARGET = $(BUILD)/prog.exe

SRC = src/main.c $(wildcard src/Timestamp/*.c) $(wildcard src/DataTypes/*/*.c)
OBJ = $(SRC:%.c=$(BUILD)/%.o)

# Benchmark: tests/bench.c + os .c de src/ EXCETO src/main.c
# (duas main() quebrariam o link). tests/cpu_detect.h e header-only.
BENCH_SRC = tests/bench.c $(wildcard src/Timestamp/*.c) $(wildcard src/DataTypes/*/*.c)
BENCH_OBJ = $(BENCH_SRC:%.c=$(BUILD)/%.o)
BENCH     = $(BUILD)/bench.exe

# Comandos de shell: funcionam tanto no Linux (sh) quanto no Windows (cmd).
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

# Compila e roda o benchmark. Argumentos opcionais: make bench ARGS="quick"
bench: $(BENCH)
	@$(call RUN,$(BENCH)) $(ARGS)

clean:
	@$(call RMDIR,$(BUILD))

-include $(OBJ:.o=.d) $(BENCH_OBJ:.o=.d)

.PHONY: run bench clean
