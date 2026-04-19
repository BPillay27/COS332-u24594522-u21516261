CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra
LDFLAGS = 

TARGET = bot

SOURCES = $(wildcard *.cpp)
OBJECTS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean

run: $(TARGET)
	./$(TARGET)

zip: $(TARGET)
	zip u24594522_u21516261.zip $(TARGET) .env *.cpp *.h

rz:
	rm -f u24594522_u21516261.zip

hash: $(TARGET)
	md5sum $(TARGET)


