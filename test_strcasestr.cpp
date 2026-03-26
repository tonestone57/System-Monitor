#include <cstring>
#include <cstdio>

int main() {
    const char* haystack = "HelloWorld";
    const char* needle = "owo";
    if (strcasestr(haystack, needle)) {
        printf("Found\n");
    }
    return 0;
}
