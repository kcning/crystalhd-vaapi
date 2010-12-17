SUFFIXES += .d

SRCS = $(shell find . -type f -name \*.cpp)
OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)
EXE = h264_nalu_decoder

CFLAGS = -O2 -pipe -D_GNU_SOURCE -g
CXXFLAGS = -O2 -pipe -D_GNU_SOURCE -g

MAKEDEPEND = $(CXX) -MM $(CXXFLAGS)

.PHONY: all clean

all: $(EXE)

-include $(DEPS)

$(EXE): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(@)

clean:
	rm -f $(OBJS)
	rm -f $(DEPS)
	rm -f $(EXE)

%.d: %.cpp
	$(MAKEDEPEND) $< > $@

%.o: %.cpp %.d
