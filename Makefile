CC     = gcc
CFLAGS = -O0 -Wall -Wextra -g -MMD -MP -Isrc
BUILD  = build
TARGET = $(BUILD)/prog.exe

SRC = src/main.c $(wildcard src/Timestamp/*.c) $(wildcard src/DataTypes/*/*.c)
OBJ = $(SRC:%.c=$(BUILD)/%.o)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@

$(BUILD)/%.o: %.c
	@if not exist "$(subst /,\,$(@D))" mkdir "$(subst /,\,$(@D))"
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	@$(subst /,\,$(TARGET))

clean:
	@if exist "$(BUILD)" rmdir /s /q "$(BUILD)"

-include $(OBJ:.o=.d)

.PHONY: clean