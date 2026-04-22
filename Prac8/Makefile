CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
LDFLAGS =

TARGET = backup_client

SOURCES = $(wildcard *.cpp)
OBJECTS = $(SOURCES:.cpp=.o)

ZIPFILE = u24594522_u21516261.zip

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

zip: clean
	zip $(ZIPFILE) .env *.cpp *.h Makefile

rz:
	rm -f $(ZIPFILE)

hash: $(TARGET)
	md5sum $(TARGET)

.PHONY: all clean run zip rz hash