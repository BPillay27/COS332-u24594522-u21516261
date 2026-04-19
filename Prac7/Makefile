CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra
LDFLAGS = 

TARGET = bot

SOURCES = $(wildcard *.cpp)
OBJECTS = $(SOURCES:.cpp=.o)

USER = violetabyss
SUBJECT = prac7

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

email: 
	@echo "Sending test email to $(USER)..."
	@printf "HELO localhost\r\n\
	MAIL FROM:<tester@localhost>\r\n\
	RCPT TO:<$(USER)@localhost>\r\n\
	DATA\r\n\
	Subject: $(SUBJECT)\r\n\
	\r\n\
	This is an automated test message for the vacation bot.\r\n\
	.\r\n\
	QUIT\r\n" | nc -C localhost 25
	@echo "Email sent. Run your bot to check for replies."
