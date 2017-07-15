
#ifndef TAISEI_VFS_ZIPFILE
#define TAISEI_VFS_ZIPFILE

#include "private.h"
#include "hashtable.h"

typedef struct VFSZipFileTLS {
    zip_t *zip;
    SDL_RWops *stream;
    zip_error_t error;
} VFSZipFileTLS;

typedef struct VFSZipFileData {
    VFSNode *source;
    Hashtable *pathmap;
    SDL_TLSID tls_id;
} VFSZipFileData;

typedef struct VFSZipFileIterData {
    zip_int64_t idx;
    zip_int64_t num;
    const char *prefix;
    size_t prefix_len;
    char *allocated;
} VFSZipFileIterData;

bool vfs_zipfile_init(VFSNode *node, VFSNode *source);
const char* vfs_zipfile_iter_shared(VFSNode *node, VFSZipFileData *zdata, VFSZipFileIterData *idata, VFSZipFileTLS *tls);
void vfs_zipfile_iter_stop(VFSNode *node, void **opaque);

#endif
