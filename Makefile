CXX := clang++
CXXFLAGS := -g -std=c++23
CPPFLAGS :=
LDFLAGS :=
LDLIBS := -lvulkan -lglfw
FORMAT := clang-format
SLANGC := slangc

ARTIFACTS_DIR := .artifacts
TARGET := $(ARTIFACTS_DIR)/app
SRC := main.cpp vulkan.cpp
OBJ := $(ARTIFACTS_DIR)/main.o $(ARTIFACTS_DIR)/vulkan.o
DB_FRAGMENT := $(SRC:%.cpp=$(ARTIFACTS_DIR)/%.json)
COMPILE_COMMANDS := $(ARTIFACTS_DIR)/compile_commands.json
MODULE_PCM := $(ARTIFACTS_DIR)/vulkan.pcm
SHADER_SRC := shader.slang
SHADER_SPV := $(ARTIFACTS_DIR)/slang.spv
SHADER_FLAGS := -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name -entry vertMain -entry fragMain

.DEFAULT_GOAL := all
.PHONY: all build run clean format shaders
.DELETE_ON_ERROR:

all: build

build: $(TARGET) $(COMPILE_COMMANDS)

shaders: $(SHADER_SPV)

$(SHADER_SPV): $(SHADER_SRC) Makefile
	mkdir -p $(ARTIFACTS_DIR)
	$(SLANGC) $< $(SHADER_FLAGS) -o $@

$(ARTIFACTS_DIR)/vulkan.o $(MODULE_PCM) &: vulkan.cpp $(SHADER_SPV) Makefile
	mkdir -p $(ARTIFACTS_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -x c++-module -fmodule-output=$(MODULE_PCM) -MJ $(ARTIFACTS_DIR)/vulkan.json -c $< -o $(ARTIFACTS_DIR)/vulkan.o

$(ARTIFACTS_DIR)/main.o: main.cpp $(MODULE_PCM) Makefile
	mkdir -p $(ARTIFACTS_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -fprebuilt-module-path=$(ARTIFACTS_DIR) -MJ $(ARTIFACTS_DIR)/main.json -c $< -o $@

$(TARGET): $(OBJ)
	mkdir -p $(ARTIFACTS_DIR)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(COMPILE_COMMANDS): $(DB_FRAGMENT)
	mkdir -p $(ARTIFACTS_DIR)
	printf '[\n' > $@
	sed '$$s/,$$//' $^ >> $@
	printf '\n]\n' >> $@

run: build
	./$(TARGET)

format:
	$(FORMAT) -i $(SRC)

clean:
	rm -rf $(ARTIFACTS_DIR)
