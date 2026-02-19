
CXX = g++
CXXFLAGS = -O3 -std=c++11 -Wall

TARGETS = benchmark_process_map benchmark_vector_resize_with_reserve

all: $(TARGETS)

benchmark_process_map: benchmark_process_map.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

benchmark_vector_resize_with_reserve: benchmark_vector_resize_with_reserve.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)
