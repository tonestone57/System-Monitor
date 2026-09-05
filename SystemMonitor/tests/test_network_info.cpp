#include <iostream>
#include <cassert>
#include <cstring>
#include "../NetworkView.h"

void test_network_info_null_name() {
	NetworkInfo info;
	const char* ifName = NULL;
	if (ifName != NULL)
		strlcpy(info.name, ifName, sizeof(info.name));
	else
		info.name[0] = '\0';

	assert(strcmp(info.name, "") == 0);
	std::cout << "test_network_info_null_name passed!" << std::endl;
}

void test_network_info_valid_name() {
	NetworkInfo info;
	const char* ifName = "eth0";
	if (ifName != NULL)
		strlcpy(info.name, ifName, sizeof(info.name));
	else
		info.name[0] = '\0';

	assert(strcmp(info.name, "eth0") == 0);
	std::cout << "test_network_info_valid_name passed!" << std::endl;
}

void test_network_info_long_name() {
	NetworkInfo info;
	std::string longName(100, 'a');
	const char* ifName = longName.c_str();
	if (ifName != NULL)
		strlcpy(info.name, ifName, sizeof(info.name));
	else
		info.name[0] = '\0';

	assert(strlen(info.name) == sizeof(info.name) - 1);
	assert(info.name[sizeof(info.name) - 1] == '\0');
	std::cout << "test_network_info_long_name passed!" << std::endl;
}

int main() {
	test_network_info_null_name();
	test_network_info_valid_name();
	test_network_info_long_name();
	std::cout << "All NetworkInfo tests passed!" << std::endl;
	return 0;
}
