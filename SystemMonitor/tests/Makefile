
CXX = g++
CXXFLAGS = -O3 -std=c++11 -Wall

TARGETS = benchmark_process_map benchmark_thread_scanning

all: $(TARGETS)

benchmark_process_map: benchmark_process_map.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

benchmark_thread_scanning: benchmark_thread_scanning.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)
