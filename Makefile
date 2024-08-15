# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++11
CXXFLAGS += -Wall -g

# Libraries
<<<<<<< HEAD
LIBS = -lssl -lcrypto -lboost_system -lboost_filesystem -lz -lminizip -pthread
=======
LIBS = -lssl -lcrypto -lboost_system -lboost_filesystem -lz -lminizip
>>>>>>> 0a525aac6cf25fc80044469ea9b1a7edfccaaf57

# Directories
SRC_DIR = . # Define your source directory
BUILD_DIR = build

# Executable target
TARGET = $(BUILD_DIR)/agent

# Find all .cpp files recursively in the source directory
SRCS = $(shell find $(SRC_DIR) -name '*.cpp')

# Create a list of object files, converting .cpp paths to .o paths in the build directory
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))

# Default target
all: $(TARGET)

# Rule to build the executable
$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LIBS)

# General rule to compile .cpp files into .o files, including subdirectories
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@mkdir -p $(@D) # Create the directory for the object file
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Ensure build directory exists
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Clean rule to remove compiled files
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

