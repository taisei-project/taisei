
#ifndef TAISEI_VFS_PUBLIC
#define TAISEI_VFS_PUBLIC

#include <SDL.h>
#include <stdbool.h>

typedef struct VFSInfo {
    unsigned int error: 1;
    unsigned int exists : 1;
    unsigned int is_dir : 1;
} VFSInfo;

#define VFSINFO_ERROR ((VFSInfo){.error = true, 0})

typedef enum VFSOpenMode {
    VFS_MODE_READ = 1,
    VFS_MODE_WRITE = 2,
} VFSOpenMode;

typedef struct VFSDir VFSDir;

SDL_RWops* vfs_open(const char *path, VFSOpenMode mode);
VFSInfo vfs_query(const char *path);
bool vfs_mkdir(const char *path);
void vfs_mkdir_required(const char *path);

bool vfs_create_union_mountpoint(const char *mountpoint);
bool vfs_mount_syspath(const char *mountpoint, const char *fspath, bool mkdir);

VFSDir* vfs_dir_open(const char *path);
void vfs_dir_close(VFSDir *dir);
const char* vfs_dir_read(VFSDir *dir);

char* vfs_syspath(const char *path);
char* vfs_syspath_or_repr(const char *path);
bool vfs_print_tree(SDL_RWops *dest, char *path);

// these are defined in private.c, but need to be accessible from external code
void vfs_init(void);
void vfs_uninit(void);
const char* vfs_get_error(void);

#endif
