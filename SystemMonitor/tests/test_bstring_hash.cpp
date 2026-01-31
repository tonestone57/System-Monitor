#include <String.h>
#include <unordered_map>
#include <stdio.h>

// Mocking if BString doesn't have std::hash specialization
namespace std {
    template<> struct hash<BString> {
        size_t operator()(const BString& s) const {
            size_t hash = 5381;
            const char* str = s.String();
            int c;
            while ((c = *str++))
                hash = ((hash << 5) + hash) + c;
            return hash;
        }
    };
}

int main() {
    std::unordered_map<BString, int> map;
    map["test"] = 1;
    if (map["test"] == 1) printf("Success\n");
    return 0;
}
