
CXX = g++
CXXFLAGS = -O3 -std=c++11 -Wall -Wno-multichar -I. -Imocks

TARGETS = benchmark_disk_poll benchmark_process_map benchmark_thread_scanning benchmark_vector_resize_with_reserve benchmark_active_skip benchmark_activity_draw benchmark_activity_valueat benchmark_activity_getvalues test_circular_buffer test_utils test_data_history test_format_bytes benchmark_battery_cache benchmark_process_view_visibility benchmark_process_filter test_cpu_usage test_disk_list_item test_process_list_item test_network_info test_strlcpy_bounds test_process_filter_safety

all: $(TARGETS)

test_network_info_strlcpy: test_network_info_strlcpy.cpp HaikuMocks.o
	$(CXX) $(CXXFLAGS) -I.. -o $@ $^

HaikuMocks.o: HaikuMocks.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

Utils.o: ../Utils.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

test_format_bytes: test_format_bytes.cpp Utils.o HaikuMocks.o
	$(CXX) $(CXXFLAGS) -o $@ $^

benchmark_activity_valueat: benchmark_activity_valueat.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

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

benchmark_activity_getvalues: benchmark_activity_getvalues.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

test_circular_buffer: test_circular_buffer.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

test_utils: test_utils.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< ../Utils.cpp

benchmark_disk_poll: benchmark_disk_poll.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

test_data_history: test_data_history.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

benchmark_battery_cache: benchmark_battery_cache.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

benchmark_process_view_visibility: benchmark_process_view_visibility.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS) *.o

benchmark_process_filter: benchmark_process_filter.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

test_cpu_usage: test_cpu_usage.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

test_disk_list_item: test_disk_list_item.cpp ../Utils.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

test_process_list_item: test_process_list_item.cpp ../Utils.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

test_network_info: test_network_info.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

test_strlcpy_bounds: test_strlcpy_bounds.cpp
	$(CXX) $(CXXFLAGS) -I.. -o $@ $^

test_process_filter_safety: test_process_filter_safety.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<
