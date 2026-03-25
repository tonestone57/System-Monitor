#ifndef HAIKU_MOCKS_H
#define HAIKU_MOCKS_H

#include <string>
#include <cstdint>
#include <cstdarg>
#include <vector>

typedef uint64_t uint64;
typedef uint32_t uint32;
typedef int32_t int32;
typedef int64_t bigtime_t;

#define B_TRANSLATE(x) x
#define B_OK 0
#define B_MAIN_SCREEN_ID 0
#define IFF_LOOPBACK 1
#define IFF_UP 2
#define AF_INET 2
#define B_PAGE_SIZE 4096
#define B_BEOS_LIB_DIRECTORY 1
#define B_TOPOLOGY_CORE 1
#define B_PANEL_BACKGROUND_COLOR 1
#define B_SIZE_UNSET -1
#define B_ALIGN_LEFT 0

class BString {
public:
    std::string str;

    BString() {}
    BString(const char* s) : str(s ? s : "") {}
    BString(const BString& other) : str(other.str) {}
    BString(const char* s, int len) : str(s, len) {}

    const char* String() const { return str.c_str(); }
    int Length() const { return str.length(); }
    bool IsEmpty() const { return str.empty(); }

    BString& SetToFormat(const char* format, ...) {
        va_list args;
        va_start(args, format);
        int len = vsnprintf(nullptr, 0, format, args);
        va_end(args);

        if (len > 0) {
            std::vector<char> buf(len + 1);
            va_start(args, format);
            vsnprintf(buf.data(), buf.size(), format, args);
            va_end(args);
            str = buf.data();
        }
        return *this;
    }

    BString& operator=(const BString& other) { str = other.str; return *this; }
    BString& operator=(const char* s) { str = s; return *this; }
    BString& operator<<(const char* s) { str += s; return *this; }
    BString& operator<<(int i) { str += std::to_string(i); return *this; }
    BString& operator<<(uint64_t i) { str += std::to_string(i); return *this; }
    BString& operator<<(double d) { str += std::to_string(d); return *this; }

    BString& Append(const char* s) { str += s; return *this; }

    void Truncate(int newLen, bool lazy = true) {
        if (newLen < (int)str.length()) str.resize(newLen);
    }

    int32 FindFirst(const char* s, int32 offset = 0) const {
        size_t pos = str.find(s, offset);
        return pos == std::string::npos ? -1 : pos;
    }

    void CopyInto(BString& into, int32 offset, int32 length) const {
        into.str = str.substr(offset, length);
    }

    void Trim() {}

    bool EndsWith(const char* suffix) const {
        std::string s(suffix);
        if (str.length() >= s.length()) {
            return (0 == str.compare(str.length() - s.length(), s.length(), s));
        }
        return false;
    }

    bool operator==(const BString& other) const { return str == other.str; }
    bool operator==(const char* other) const { return str == other; }
};

#define B_PRIu64 "llu"
#define B_PRId64 "lld"
#define B_HAIKU_ABI_NAME "mock_abi"

class BHandler {
public:
    virtual ~BHandler() {}
};

class BWindow {};

class BPoint {
public:
    float x, y;
};

class BSize {
public:
    BSize(float w, float h) {}
};

inline int ui_color(int) { return 0; }

struct system_info {
    uint32 max_pages;
    uint32 used_pages;
    uint32 cached_pages;
    uint32 block_cache_pages;
    uint32 ignored_pages;
    uint32 max_swap_pages;
    uint32 used_swap_pages;
    int page_size;
    uint32 cpu_clock_speed;
    uint32 cpu_type;
    uint32 cpu_count;
    char kernel_version[256];
};

inline int get_system_info(system_info* info) {
    if(info) {
        info->max_pages = 0;
        info->used_pages = 0;
        info->cached_pages = 0;
        info->block_cache_pages = 0;
        info->ignored_pages = 0;
        info->max_swap_pages = 0;
        info->used_swap_pages = 0;
        info->page_size = 4096;
        info->cpu_clock_speed = 0;
        info->cpu_type = 0;
        info->cpu_count = 1;
        info->kernel_version[0] = '\0';
    }
    return B_OK;
}

inline bigtime_t system_time() { return 0; }

struct cpu_topology_node_info {
    int type;
    union {
        struct {
            uint64 default_frequency;
        } core;
    } data;
};

inline int get_cpu_topology_info(cpu_topology_node_info* nodes, uint32* count) {
    if (count) *count = 0;
    return B_OK;
}

struct accelerant_device_info {
    uint32 version;
    char name[32];
    char chipset[32];
    char serial_no[32];
};

struct display_mode {
    struct {
        uint32 pixel_clock;
        uint32 h_total;
        uint32 v_total;
    } timing;
    uint32 virtual_width;
    uint32 virtual_height;
};

class BScreen {
public:
    BScreen(int id) {}
    bool IsValid() { return true; }
    int GetDeviceInfo(accelerant_device_info* info) { return 0; }
    int GetMode(display_mode* mode) { return 0; }
};

class BFont {
public:
    float Size() const { return 12.0f; }
};
extern BFont* be_bold_font;

class BMessage {
public:
    BMessage(int) {}
    int AddInt32(const char*, int) { return 0; }
};

class BNetworkAddress {
public:
    int Family() const { return AF_INET; }
    BString ToString() const { return "mock_ip"; }
};

#endif // HAIKU_MOCKS_H
