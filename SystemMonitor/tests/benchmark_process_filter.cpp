#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cinttypes>
#include <cstdarg>

// Simple mock for benchmarking
class BString {
public:
    std::string data;
    BString() {}
    BString(const char* str) : data(str ? str : "") {}
    void SetTo(const char* str) { data = str ? str : ""; }
    void SetToFormat(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        int len = vsnprintf(nullptr, 0, fmt, args);
        va_end(args);

        if (len >= 0) {
            std::vector<char> buf(len + 1);
            va_start(args, fmt);
            vsnprintf(buf.data(), buf.size(), fmt, args);
            va_end(args);
            data = buf.data();
        }
    }
    int IFindFirst(const char* str) const {
        if (!str || !*str) return 0;
        const char* res = strcasestr(data.c_str(), str);
        if (res) return res - data.c_str();
        return -1;
    }
};

#define B_ERROR -1
#define B_OS_NAME_LENGTH 32
#define B_PRId32 PRId32

typedef int32_t team_id;

struct ProcessInfo {
	team_id id;
	char name[B_OS_NAME_LENGTH];
	char userName[B_OS_NAME_LENGTH];
	char args[64];
};

class ProcessView {
public:
	BString fFilterName;
	BString fFilterID;
	BString fFilterArgs;

	bool _MatchesFilter_Old(const ProcessInfo& info, const char* searchText) {
		if (searchText == NULL || strlen(searchText) == 0)
			return true;

		fFilterName.SetTo(info.name);
		fFilterID.SetToFormat("%" B_PRId32, info.id);
		fFilterArgs.SetTo(info.args);

		if (fFilterName.IFindFirst(searchText) != B_ERROR
			|| fFilterID.IFindFirst(searchText) != B_ERROR
			|| fFilterArgs.IFindFirst(searchText) != B_ERROR) {
			return true;
		}
		return false;
	}

	bool _MatchesFilter_New(const ProcessInfo& info, const char* searchText) {
		if (searchText == NULL || searchText[0] == '\0')
			return true;

		if (strcasestr(info.name, searchText) != NULL)
			return true;

		if (strcasestr(info.args, searchText) != NULL)
			return true;

		BString idStr;
		idStr.SetToFormat("%" B_PRId32, info.id);
		if (strcasestr(idStr.data.c_str(), searchText) != NULL)
			return true;

		return false;
	}
};

int main() {
    std::vector<ProcessInfo> procs;
    for (int i = 0; i < 1000; i++) {
        ProcessInfo info;
        info.id = 1000 + i;
        snprintf(info.name, sizeof(info.name), "Process_%d", i);
        snprintf(info.userName, sizeof(info.userName), "User_%d", i % 5);
        snprintf(info.args, sizeof(info.args), "--arg %d", i);
        procs.push_back(info);
    }

    ProcessView view;

    int iterations = 10000;
    std::vector<const char*> queries = {"500", "Process", "--arg", "NonExistentString"};

    for (const char* searchText : queries) {
        std::cout << "Query: '" << searchText << "'\n";

        int matchCountOld = 0;
        auto startOld = std::chrono::high_resolution_clock::now();
        for (int it = 0; it < iterations; it++) {
            for (const auto& p : procs) {
                if (view._MatchesFilter_Old(p, searchText)) {
                    matchCountOld++;
                }
            }
        }
        auto endOld = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> oldTime = endOld - startOld;

        int matchCountNew = 0;
        auto startNew = std::chrono::high_resolution_clock::now();
        for (int it = 0; it < iterations; it++) {
            for (const auto& p : procs) {
                if (view._MatchesFilter_New(p, searchText)) {
                    matchCountNew++;
                }
            }
        }
        auto endNew = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> newTime = endNew - startNew;

        std::cout << "  Old method time: " << oldTime.count() << " ms (Matches: " << matchCountOld << ")\n";
        std::cout << "  New method time: " << newTime.count() << " ms (Matches: " << matchCountNew << ")\n";
        std::cout << "  Improvement: " << (1.0 - newTime.count() / oldTime.count()) * 100 << "%\n";

        if (matchCountOld != matchCountNew) {
            std::cerr << "Mismatch in match count!\n";
            return 1;
        }
    }

    return 0;
}
