# g++ -Wall -g -c -o build/main.o main.cpp
# g++ -Wall -g -o build/grabber build/main.o -lboost_system -lboost_filesystem -lz -lminizip

# Makefile for building screenshot program

# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -g

# Libraries
LIBS = -lboost_system -lboost_filesystem -lz -lminizip

# Directories
BUILD_DIR = build

# Executable and source files
TARGET = $(BUILD_DIR)/grabber
SRCS = main.cpp
OBJS = $(BUILD_DIR)/main.o

# Default target
all: $(TARGET)

# Create build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Link the executable
$(TARGET): $(BUILD_DIR) $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LIBS)

# Compile the source file into an object file
$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Clean up
clean:
	rm -f $(TARGET) $(OBJS)