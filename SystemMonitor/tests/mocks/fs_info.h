#ifndef MOCK_FS_INFO_H
#define MOCK_FS_INFO_H
#include <sys/types.h>
struct fs_info {
    long long total_blocks;
    long long free_blocks;
    long block_size;
    char volume_name[256];
    char fsh_name[256];
};
inline int fs_stat_dev(dev_t dev, fs_info* info) {
    if(info) {
        info->total_blocks = 1000;
        info->free_blocks = 500;
        info->block_size = 1024;
        info->fsh_name[0] = 'b'; info->fsh_name[1] = 'f'; info->fsh_name[2] = 's'; info->fsh_name[3] = '\0';
        info->volume_name[0] = 'H'; info->volume_name[1] = 'a'; info->volume_name[2] = 'i'; info->volume_name[3] = 'k'; info->volume_name[4] = 'u'; info->volume_name[5] = '\0';
    }
    return 0;
}
inline dev_t dev_for_path(const char* path) { return 1; }
#endif
