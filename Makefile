CC = gcc
CXXFLAGS = -std=c++17 -O3 -pthread
LDFLAGS = -lstdc++ -pthread
TARGET = main
SRC = main.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CXXFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET)
