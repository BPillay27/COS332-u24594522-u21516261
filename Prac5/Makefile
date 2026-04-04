CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra
LDFLAGS =

TARGET = ldap_client

SOURCES = main.cpp LDAPClient.cpp
OBJECTS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp LDAPClient.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

zip:
	zip u24594522_u21516261.zip $(TARGET) *.cpp *.h Makefile

rz:
	rm -f u24594522_u21516261.zip

hash: $(TARGET)
	md5sum $(TARGET)

.PHONY: all clean run zip rz hash