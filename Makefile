SRCS = GF28Value.cc RScodeTest.cc

OBJS = $(SRCS:.cc=.o)

TARGET = RScodeTest

%.o: *%.cc
	$(CXX) -g -c -Wall --std=c++11 $(CXXFLAGS) $<

$(TARGET): $(OBJS)
	$(CXX) -o $(TARGET) $(OBJS)

.PHONY: all clean

all: $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
	