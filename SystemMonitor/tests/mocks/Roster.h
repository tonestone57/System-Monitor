#ifndef MOCK_ROSTER_H
#define MOCK_ROSTER_H
struct app_info {
    int ref;
};
class BRoster {
public:
    int GetAppInfo(const char*, app_info*) { return 0; }
};
extern BRoster* be_roster;
#endif
