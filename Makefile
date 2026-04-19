CXX := clang++
CXXFLAGS := -g -std=c++23
CPPFLAGS :=
LDFLAGS :=
LDLIBS := -lvulkan -lglfw

ARTIFACTS_DIR := .artifacts
TARGET := $(ARTIFACTS_DIR)/app
SRC := main.cpp
OBJ := $(ARTIFACTS_DIR)/main.o
DB_FRAGMENT := $(ARTIFACTS_DIR)/main.json
COMPILE_COMMANDS := $(ARTIFACTS_DIR)/compile_commands.json

.DEFAULT_GOAL := all
.PHONY: all build run clean
.DELETE_ON_ERROR:

all: build

build: $(TARGET) $(COMPILE_COMMANDS)

$(OBJ): $(SRC) Makefile
	mkdir -p $(ARTIFACTS_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MJ $(DB_FRAGMENT) -c $< -o $@

$(TARGET): $(OBJ)
	mkdir -p $(ARTIFACTS_DIR)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(COMPILE_COMMANDS): $(DB_FRAGMENT)
	mkdir -p $(ARTIFACTS_DIR)
	printf '[\n' > $@
	sed '$$s/,$$//' $^ >> $@
	printf '\n]\n' >> $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(ARTIFACTS_DIR)
