CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
TARGET   := tokenizer
SRC      := main.cpp

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)
