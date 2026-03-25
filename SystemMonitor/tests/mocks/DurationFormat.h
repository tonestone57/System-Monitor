#ifndef MOCK_DURATION_FORMAT_H
#define MOCK_DURATION_FORMAT_H
class BDurationFormat {
public:
    BDurationFormat() {}
    void SetSeparator(const BString&) {}
    int Format(BString& str, bigtime_t start, bigtime_t end) {
        str = "mock_duration";
        return 0;
    }
};
#endif
