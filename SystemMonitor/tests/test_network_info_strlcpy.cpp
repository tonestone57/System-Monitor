#include <cassert>
#include <cstring>
#include <iostream>
#include <string>

// Include NetworkView.h or replicate NetworkInfo definition for testing
#include "NetworkView.h"

void test_network_info_strlcpy_truncation()
{
	NetworkInfo info = {};

	// 1. Test interface name copy with excess size
	std::string longName(200, 'a');
	const char* ifName = longName.c_str();
	strlcpy(info.name, ifName != nullptr ? ifName : "", sizeof(info.name));
	assert(strlen(info.name) == sizeof(info.name) - 1);
	assert(info.name[sizeof(info.name) - 1] == '\0');

	// 2. Test type string copy with excess size
	std::string longType(100, 'b');
	const char* typeCStr = longType.c_str();
	strlcpy(info.typeStr, typeCStr != nullptr ? typeCStr : "", sizeof(info.typeStr));
	assert(strlen(info.typeStr) == sizeof(info.typeStr) - 1);
	assert(info.typeStr[sizeof(info.typeStr) - 1] == '\0');

	// 3. Test address string copy with excess size
	std::string longAddr(300, 'c');
	const char* addrCStr = longAddr.c_str();
	strlcpy(info.addressStr, addrCStr != nullptr ? addrCStr : "", sizeof(info.addressStr));
	assert(strlen(info.addressStr) == sizeof(info.addressStr) - 1);
	assert(info.addressStr[sizeof(info.addressStr) - 1] == '\0');

	// 4. Test null pointer handling
	const char* nullStr = nullptr;
	strlcpy(info.name, nullStr != nullptr ? nullStr : "", sizeof(info.name));
	assert(strlen(info.name) == 0);
	assert(info.name[0] == '\0');

	std::cout << "test_network_info_strlcpy passed!" << std::endl;
}

int main()
{
	test_network_info_strlcpy_truncation();
	return 0;
}
