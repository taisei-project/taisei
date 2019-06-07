/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "android_assets_public.h"
#include "vfs_android.h"
#include "private.h"

// SDL internal functions, from SDL_android.h
int Android_JNI_FileOpen(SDL_RWops* ctx, const char* fileName, const char* mode);
int Android_JNI_FileOpen(SDL_RWops* ctx, const char* fileName, const char* mode);
Sint64 Android_JNI_FileSize(SDL_RWops* ctx);
Sint64 Android_JNI_FileSeek(SDL_RWops* ctx, Sint64 offset, int whence);
size_t Android_JNI_FileRead(SDL_RWops* ctx, void* buffer, size_t size, size_t maxnum);
size_t Android_JNI_FileWrite(SDL_RWops* ctx, const void* buffer, size_t size, size_t num);
int Android_JNI_FileClose(SDL_RWops* ctx);

#define _path_ data1

static VFSNodeFuncs vfs_funcs_assets;

static VFSNode *make_asset_node(const char *path) {
	VFSNode *n = vfs_alloc();
	n->funcs = &vfs_funcs_assets;
	n->_path_ = vfs_path_normalize_alloc(path);
	return n;
}

static char *vfs_assets_repr(VFSNode *n) {
	return strfmt("android asset: %s", n->_path_);
}

static VFSInfo vfs_assets_query(VFSNode *n) {
	if(n->data2) {
		return *(VFSInfo*)n->data2;
	}

	VFSAndroidTLS *tls = vfs_android_get_tls();
	JNIEnv *jni = tls->jni;
	jstring pstr = (*jni)->NewStringUTF(jni, n->_path_);
	jint qresult = (*jni)->CallIntMethod(jni, tls->activity.obj, tls->activity.methods.queryAssetPath, pstr);
	(*jni)->DeleteLocalRef(jni, pstr);

	VFSInfo i = {
		.exists = qresult != 0,
		.is_dir = qresult == 2,
		.is_readonly = true,
	};

	n->data2 = memdup(&i, sizeof(i));
	return i;
}

static void vfs_assets_free(VFSNode *n) {
	free(n->data1);
	free(n->data2);
}

static VFSNode *vfs_assets_locate(VFSNode *n, const char *path) {
	char fullpath[strlen(n->_path_) + strlen(path) + 2];
	snprintf(fullpath, sizeof(fullpath), "%s"VFS_PATH_SEPARATOR_STR"%s", n->_path_, path);
	return make_asset_node(fullpath);
}

typedef struct AssetIter {
	jobjectArray array;
	jint idx;
	jint numitems;
	char *str;
} AssetIter;

static const char *vfs_assets_iter(VFSNode *n, void **opaque) {
	AssetIter *iter = *opaque;
	VFSAndroidTLS *tls = vfs_android_get_tls();
	JNIEnv *jni = SDL_AndroidGetJNIEnv();

	if(iter == NULL) {
		iter = *opaque = calloc(1, sizeof(AssetIter));
		jstring pstr = (*jni)->NewStringUTF(jni, n->_path_);
		iter->array = (*jni)->CallObjectMethod(jni, tls->assetManager.obj, tls->assetManager.methods.list, pstr);

		jthrowable ex = (*jni)->ExceptionOccurred(jni);
		if(ex) {
			(*jni)->ExceptionClear(jni);
			(*jni)->DeleteLocalRef(jni, ex);
			assume(iter->array == NULL);
		}

		(*jni)->DeleteLocalRef(jni, pstr);

		if(iter->array == NULL) {
			vfs_set_error("%s: not a directory", n->_path_);
			return NULL;
		}

		iter->numitems = (*jni)->GetArrayLength(jni, iter->array);
		vfs_android_make_ref_global(&iter->array);
	}

	if(iter->idx >= iter->numitems) {
		return NULL;
	}

	jstring str = (*jni)->GetObjectArrayElement(jni, iter->array, iter->idx);
	const char *utf = (*jni)->GetStringUTFChars(jni, str, 0);
	stralloc(&iter->str, utf);
	(*jni)->ReleaseStringUTFChars(jni, str, utf);
	(*jni)->DeleteLocalRef(jni, str);
	++iter->idx;

	return iter->str;
}

static void vfs_assets_iter_stop(VFSNode *n, void **opaque) {
	AssetIter *iter = *opaque;

	if(iter) {
		if(iter->array) {
			JNIEnv *jni = SDL_AndroidGetJNIEnv();
			(*jni)->DeleteGlobalRef(jni, iter->array);
		}

		free(iter->str);
		free(iter);
		*opaque = NULL;
	}
}

static SDL_RWops *vfs_assets_open(VFSNode *n, VFSOpenMode mode) {
	if((mode & VFS_MODE_WRITE) || !(mode & VFS_MODE_READ)) {
		vfs_set_error("Android assets are read-only");
		return NULL;
	}

	SDL_RWops *rw = SDL_AllocRW();
	assume(rw != NULL);

	if(Android_JNI_FileOpen(rw, n->_path_, "rb") < 0) {
		SDL_FreeRW(rw);
		vfs_set_error_from_sdl();
		return NULL;
	}

	rw->size = Android_JNI_FileSize;
	rw->seek = Android_JNI_FileSeek;
	rw->read = Android_JNI_FileRead;
	rw->write = Android_JNI_FileWrite;
	rw->close = Android_JNI_FileClose;
	rw->type = SDL_RWOPS_JNIFILE;

	return rw;
}

static VFSNodeFuncs vfs_funcs_assets = {
	.repr = vfs_assets_repr,
	.query = vfs_assets_query,
	.free = vfs_assets_free,
	.locate = vfs_assets_locate,
	.iter = vfs_assets_iter,
	.iter_stop = vfs_assets_iter_stop,
	.open = vfs_assets_open,
};

bool vfs_mount_assets(const char *mountpoint, const char *apath) {
	VFSNode *anode = make_asset_node(apath);

	if(!vfs_node_query(anode).exists) {
		vfs_set_error("Asset %s does not exist", apath);
		vfs_decref(anode);
		return false;
	}

	if(!vfs_mount(vfs_root, mountpoint, anode)) {
		vfs_decref(anode);
		return false;
	}

	return true;
}
