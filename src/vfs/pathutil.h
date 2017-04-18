
#ifndef TAISEI_VFS_PATHUTIL
#define TAISEI_VFS_PATHUTIL

#include <SDL_platform.h>

#ifdef __WINDOWS__
    #define VFS_PATH_SEPS "/\\"
#else
    #define VFS_PATH_SEPS "/"
#endif

#define VFS_PATH_SEP VFS_PATH_SEPS[0]

char* vfs_path_normalize(const char *path, char *out);
char* vfs_path_normalize_alloc(const char *path);
char* vfs_path_normalize_inplace(char *path);
void vfs_path_split_left(char *path, char **lpath, char **rpath);
void vfs_path_split_right(char *path, char **lpath, char **rpath);
void vfs_path_root_prefix(char *path);

#endif
