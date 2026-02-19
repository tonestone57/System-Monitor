#include <kernel/OS.h>
#include <stdio.h>
int main() {
    system_info info;
    get_system_info(&info);
    // This will fail to compile if used_swap_pages doesn't exist
    unsigned int dummy = info.used_swap_pages;
    printf("%u\n", dummy);
    return 0;
}
