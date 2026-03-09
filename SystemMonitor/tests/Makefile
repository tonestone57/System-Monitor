
CXX = g++
CXXFLAGS = -O3 -std=c++11 -Wall

TARGETS = benchmark_process_map benchmark_thread_scanning benchmark_vector_resize_with_reserve benchmark_active_skip benchmark_activity_draw

all: $(TARGETS)

benchmark_activity_draw: benchmark_activity_draw.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

benchmark_process_map: benchmark_process_map.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

benchmark_thread_scanning: benchmark_thread_scanning.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

benchmark_vector_resize_with_reserve: benchmark_vector_resize_with_reserve.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

benchmark_active_skip: benchmark_active_skip.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)
