CC     = gcc
CFLAGS = -O0 -Wall -Wextra -g -MMD -MP -Isrc
BUILD  = build
TARGET = $(BUILD)/prog.exe

SRC = src/main.c $(wildcard src/Timestamp/*.c) $(wildcard src/DataTypes/*/*.c)
OBJ = $(SRC:%.c=$(BUILD)/%.o)

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

$(BUILD)/%.o: %.c
	@$(call MKDIR,$(@D))
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	@$(call RUN,$(TARGET))

clean:
	@$(call RMDIR,$(BUILD))

-include $(OBJ:.o=.d)

.PHONY: run clean
