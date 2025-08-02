#include "Utils.h"

BString FormatBytes(uint64 bytes, int precision) {
    BString str;
    double kb = bytes / 1024.0;
    double mb = kb / 1024.0;
    double gb = mb / 1024.0;

    if (gb >= 1.0) {
        str.SetToFormat("%.*f GiB", precision, gb);
    } else if (mb >= 1.0) {
        str.SetToFormat("%.*f MiB", precision, mb);
    } else if (kb >= 1.0) {
        str.SetToFormat("%.*f KiB", precision, kb);
    } else {
        str.SetToFormat("%" B_PRIu64 " Bytes", bytes);
    }
    return str;
}
