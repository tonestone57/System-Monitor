
CXX = g++
CXXFLAGS = -O3 -std=c++11 -Wall

TARGETS = benchmark_process_map

all: $(TARGETS)

benchmark_process_map: benchmark_process_map.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)
