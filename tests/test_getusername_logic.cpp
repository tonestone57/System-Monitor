#include <iostream>
#include <unordered_map>
#include <string>
#include <cstring>
#include <pwd.h>
#include <sys/types.h>

// Mock Haiku types
class BString : public std::string {
public:
    using std::string::string;
    BString() : std::string() {}
    BString(const char* s) : std::string(s ? s : "") {}
    BString& operator<<(int v) { append(std::to_string(v)); return *this; }
    const char* String() const { return c_str(); }
};

// Global mock cache
std::unordered_map<uid_t, BString> fUserNameCache;

// Mock getpwuid_r
int mock_getpwuid_r(uid_t uid, struct passwd *pwd, char *buf, size_t buflen, struct passwd **result) {
    if (uid == 1234) {
        // Found
        static char name[] = "mock_user";
        pwd->pw_name = name;
        *result = pwd;
        return 0;
    }
    // Not found
    *result = NULL;
    return 0;
}

// Logic copied from ProcessView.cpp (the new version)
const BString& GetUserName(uid_t uid) {
    auto it = fUserNameCache.find(uid);
    if (it != fUserNameCache.end()) {
        return it->second;
    }

    struct passwd pwd;
    struct passwd* result = NULL;
    char buffer[1024];

    BString name;
    // Using mock_getpwuid_r instead of real one
    if (mock_getpwuid_r(uid, &pwd, buffer, sizeof(buffer), &result) == 0 && result != NULL) {
        name = result->pw_name;
    } else {
        name << uid;
    }

    return fUserNameCache.emplace(uid, name).first->second;
}

int main() {
    // Test 1: User found
    const BString& u1 = GetUserName(1234);
    if (u1 != "mock_user") {
        std::cerr << "Test 1 Failed: Expected mock_user, got " << u1 << std::endl;
        return 1;
    }

    // Test 2: User not found (fallback to ID)
    const BString& u2 = GetUserName(9999);
    if (u2 != "9999") {
        std::cerr << "Test 2 Failed: Expected 9999, got " << u2 << std::endl;
        return 1;
    }

    // Test 3: Cache persistence (check pointers or count)
    if (fUserNameCache.size() != 2) {
        std::cerr << "Test 3 Failed: Cache size expected 2, got " << fUserNameCache.size() << std::endl;
        return 1;
    }

    // Test 4: Reference stability
    const BString& u1_again = GetUserName(1234);
    if (&u1 != &u1_again) {
         // In std::unordered_map, references to elements are stable until erasure.
         // However, rehashing might invalidate references?
         // C++11: "References and pointers to either key or data stored in the container are only invalidated by erasing that element, even when the corresponding iterator is invalidated."
         // So this check is valid.
         std::cerr << "Test 4 Failed: Reference not stable." << std::endl;
         return 1;
    }

    std::cout << "All logic tests passed." << std::endl;
    return 0;
}
