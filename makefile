# 编译器设置
CXX = g++
CXXFLAGS = -std=c++14 -Wall -Wextra -O2 -I./include

# 目录设置
SRC_DIR = src
INCLUDE_DIR = include
BIN_DIR = bin
DATA_DIR = data

# 源文件
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BIN_DIR)/%.o)

# 目标可执行文件
TARGET = $(BIN_DIR)/main.exe

# 默认目标
all: $(TARGET)

# 链接目标文件生成可执行文件
$(TARGET): $(OBJECTS) | $(BIN_DIR)
	$(CXX) $(OBJECTS) -o $@

# 编译源文件为目标文件
$(BIN_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 创建必要的目录
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	-@if exist $(BIN_DIR)\*.o del /Q $(BIN_DIR)\*.o >nul 2>&1
	-@if exist $(TARGET) del /Q $(TARGET) >nul 2>&1

distclean: clean
	-@if exist $(BIN_DIR) rmdir /S /Q $(BIN_DIR) >nul 2>&1
# 运行程序
run: $(TARGET)
	./$(TARGET) $(DATA_DIR)/network_data.csv

# 显示帮助信息
help:
	@echo "可用命令:"
	@echo "  make all     - 编译整个项目"
	@echo "  make clean   - 清理目标文件"
	@echo "  make distclean - 清理所有生成的文件"
	@echo "  make run     - 编译并运行程序"
	@echo "  make help    - 显示此帮助信息"

# 声明伪目标
.PHONY: all clean distclean run help