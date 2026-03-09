CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra
LDFLAGS = 

TARGET = program

SOURCES = main.cpp Server.cpp page.cpp
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
	./$(TARGET) 5555

zip:
	zip u24594522_u21516261.zip program *.cpp *.h

rz:
	rm -f u24594522_u21516261.zip

hash: $(TARGET)
	md5sum $(TARGET)

testNA:
	curl -i http://localhost:5555/files/

testA:
	curl -i -H "Authorization: Bearer Admin" http://localhost:5555/files/

testPost:
	curl -i -X POST http://localhost:5555/
