#include <cassert>
#include <cstdio>
#include <cstring>
#include "NetworkView.h"
#include "ProcessView.h"

void test_network_info_bounds() {
    NetworkInfo info;
    memset(&info, 0x5A, sizeof(info));

    const char* longName = "this_is_a_very_long_interface_name_that_exceeds_the_buffer_size_because_b_os_name_length_is_thirty_two_bytes";
    strlcpy(info.name, longName, sizeof(info.name));
    assert(strlen(info.name) == sizeof(info.name) - 1);
    assert(info.name[sizeof(info.name) - 1] == '\0');

    const char* longType = "this_is_a_very_long_network_type_string_that_exceeds_sixty_four_characters_limit_and_gets_truncated_properly";
    strlcpy(info.typeStr, longType, sizeof(info.typeStr));
    assert(strlen(info.typeStr) == sizeof(info.typeStr) - 1);
    assert(info.typeStr[sizeof(info.typeStr) - 1] == '\0');

    const char* longAddr = "2001:0db8:85a3:0000:0000:8a2e:0370:7334:extra_long_ipv6_address_extension_beyond_128_bytes_limit_for_testing_and_truncation_verification_when_copied";
    strlcpy(info.addressStr, longAddr, sizeof(info.addressStr));
    assert(strlen(info.addressStr) == sizeof(info.addressStr) - 1);
    assert(info.addressStr[sizeof(info.addressStr) - 1] == '\0');

    printf("test_network_info_bounds passed.\n");
}

void test_process_info_bounds() {
    ProcessInfo info;
    memset(&info, 0x5A, sizeof(info));

    const char* longName = "a_very_long_process_name_exceeding_os_name_length_limit_which_is_thirty_two_bytes";
    strlcpy(info.name, longName, sizeof(info.name));
    assert(strlen(info.name) == sizeof(info.name) - 1);
    assert(info.name[sizeof(info.name) - 1] == '\0');

    const char* longUser = "a_very_long_username_exceeding_os_name_length_limit_which_is_thirty_two_bytes";
    strlcpy(info.userName, longUser, sizeof(info.userName));
    assert(strlen(info.userName) == sizeof(info.userName) - 1);
    assert(info.userName[sizeof(info.userName) - 1] == '\0');

    const char* longArgs = "some_app --arg1=very_long_value --arg2=another_long_value --arg3=yet_another_long_value_exceeding_64_bytes";
    strlcpy(info.args, longArgs, sizeof(info.args));
    assert(strlen(info.args) == sizeof(info.args) - 1);
    assert(info.args[sizeof(info.args) - 1] == '\0');

    printf("test_process_info_bounds passed.\n");
}

int main() {
    test_network_info_bounds();
    test_process_info_bounds();
    printf("All strlcpy bounds tests passed successfully.\n");
    return 0;
}
