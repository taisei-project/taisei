
#ifndef TAISEI_VFS_ZIPPATH
#define TAISEI_VFS_ZIPPATH

#include <zip.h>

#include "private.h"
#include "zipfile.h"

typedef struct VFSZipPathData {
    VFSNode *zipnode;
    VFSZipFileTLS *tls;
    uint64_t index;
    VFSInfo info;
} VFSZipPathData;

void vfs_zippath_init(VFSNode *node, VFSNode *zipnode, VFSZipFileTLS *tls, zip_int64_t idx);

#endif
