#ifndef MOCK_PACKAGE_ROSTER_H
#define MOCK_PACKAGE_ROSTER_H
namespace BPackageKit {
    class BPackageRoster {
    public:
        void GetActivePackages(...) {}
    };
}
#define B_PACKAGE_INSTALLATION_LOCATION_SYSTEM 0
#define B_PACKAGE_INSTALLATION_LOCATION_HOME 1
class BStringList {
public:
    int CountStrings() const { return 0; }
};
#endif
