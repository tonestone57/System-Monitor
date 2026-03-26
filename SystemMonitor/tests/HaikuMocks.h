#ifndef HAIKU_MOCKS_H
#define HAIKU_MOCKS_H

#include <string>
#include <cstdint>
#include <cstdarg>
#include <vector>
#include <inttypes.h>

typedef uint64_t uint64;
typedef uint32_t uint32;
typedef int32_t int32;
typedef int64_t bigtime_t;
typedef uint8_t uint8;

#define B_TRANSLATE(x) x
#define B_OK 0
#define B_ERROR -1
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
#define B_ALIGN_RIGHT 1
#define B_WILL_DRAW 1
#define B_PULSE_NEEDED 2
#define B_NAVIGATION_BASE_COLOR (color_which)1

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t alpha;
} rgb_color;

typedef int32 color_which;

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
    bool operator!=(const char* other) const { return str != other; }
};

#define B_PRIu64 PRIu64
#define B_PRId32 PRId32
#define B_PRId64 PRId64
#define B_HAIKU_ABI_NAME "mock_abi"

class BHandler {
public:
    virtual ~BHandler() {}
};

class BWindow {
public:
    void Lock() {}
    void Unlock() {}
};

class BPoint {
public:
    BPoint() : x(0), y(0) {}
    BPoint(float x, float y) : x(x), y(y) {}
    float x, y;
};

class BRect {
public:
    BRect() : left(0), top(0), right(0), bottom(0) {}
    BRect(float l, float t, float r, float b) : left(l), top(t), right(r), bottom(b) {}
    float Width() const { return right - left; }
    float Height() const { return bottom - top; }
    float left, top, right, bottom;
};

class BSize {
public:
    BSize(float w, float h) : width(w), height(h) {}
    float width, height;
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
    uint32 used_teams;
    uint32 used_threads;
    char kernel_version[256];
};

inline int& MockCpuCount() {
    static int val = 1;
    return val;
}

inline int& MockSystemInfoResult() {
    static int val = B_OK;
    return val;
}

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
        info->cpu_count = MockCpuCount();
        info->used_teams = 0;
        info->used_threads = 0;
        info->kernel_version[0] = '\0';
    }
    return MockSystemInfoResult();
}

inline bigtime_t system_time() { return 0; }

struct cpu_info {
    bigtime_t active_time;
};

int get_cpu_info(uint32 first, uint32 count, cpu_info* info);

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
    BFont() {}
    BFont(const BFont&) {}
    BFont(const BFont* b) {}
    float Size() const { return 12.0f; }
    void SetSize(float s) {}
};
extern BFont* be_bold_font;

class BMessage {
public:
    BMessage() {}
    BMessage(int) {}
    int AddInt32(const char*, int) { return 0; }
};

class BLocker {
public:
    void Lock() {}
    void Unlock() {}
};

class BAutolock {
public:
    BAutolock(BLocker&) {}
    BAutolock(BLocker*) {}
    bool IsLocked() { return true; }
};

class BNumberFormat {
public:
    void FormatPercent(BString& out, float value) {
        out.SetToFormat("%.1f%%", value * 100.0f);
    }
};

class BNetworkAddress {
public:
    int Family() const { return AF_INET; }
    BString ToString() const { return "mock_ip"; }
};

#endif // HAIKU_MOCKS_H
