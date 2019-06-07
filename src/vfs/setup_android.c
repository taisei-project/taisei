/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "public.h"
#include "setup.h"
#include "util.h"
#include "vfs_android.h"
#include "union_public.h"
#include "loadpacks.h"
#include "android_assets_public.h"

static SDL_SpinLock vfs_android_tls_lock;
static SDL_TLSID vfs_android_tls;

void vfs_android_make_ref_global(jobject *ref) {
	VFSAndroidTLS *tls = vfs_android_get_tls();
	jobject gref = (*tls->jni)->NewGlobalRef(tls->jni, *ref);
	(*tls->jni)->DeleteLocalRef(tls->jni, *ref);
	*ref = gref;
}

static void vfs_android_tls_destructor(void *vtls) {
	VFSAndroidTLS *tls = vtls;

	(*tls->jni)->DeleteGlobalRef(tls->jni, tls->activity.obj);
	(*tls->jni)->DeleteGlobalRef(tls->jni, tls->activity.cls);
	(*tls->jni)->DeleteGlobalRef(tls->jni, tls->assetManager.obj);
	(*tls->jni)->DeleteGlobalRef(tls->jni, tls->assetManager.cls);

	free(tls);
}

VFSAndroidTLS *vfs_android_get_tls(void) {
	if(!vfs_android_tls) {
		SDL_AtomicLock(&vfs_android_tls_lock);
		if(!vfs_android_tls) {
			vfs_android_tls = SDL_TLSCreate();
		}
		SDL_AtomicUnlock(&vfs_android_tls_lock);
	}

	VFSAndroidTLS *tls = SDL_TLSGet(vfs_android_tls);

	if(tls != NULL) {
		return tls;
	}

	tls = calloc(1, sizeof(*tls));
	tls->jni = SDL_AndroidGetJNIEnv();
	tls->activity.obj = SDL_AndroidGetActivity();
	tls->activity.cls = (*tls->jni)->GetObjectClass(tls->jni, tls->activity.obj);
	tls->activity.methods.getAssets = (*tls->jni)->GetMethodID(tls->jni, tls->activity.cls, "getAssets", "()Landroid/content/res/AssetManager;");
	tls->activity.methods.queryAssetPath = (*tls->jni)->GetMethodID(tls->jni, tls->activity.cls, "queryAssetPath", "(Ljava/lang/String;)I");
	tls->assetManager.obj = (*tls->jni)->CallObjectMethod(tls->jni, tls->activity.obj, tls->activity.methods.getAssets);
	tls->assetManager.cls = (*tls->jni)->GetObjectClass(tls->jni, tls->assetManager.obj);
	tls->assetManager.methods.list = (*tls->jni)->GetMethodID(tls->jni, tls->assetManager.cls, "list", "(Ljava/lang/String;)[Ljava/lang/String;");

	SDL_TLSSet(vfs_android_tls, tls, vfs_android_tls_destructor);

	vfs_android_make_ref_global(&tls->activity.obj);
	vfs_android_make_ref_global(&tls->activity.cls);
	vfs_android_make_ref_global(&tls->assetManager.obj);
	vfs_android_make_ref_global(&tls->assetManager.cls);

	return tls;
}

static char *get_cache_path(void) {
	VFSAndroidTLS *tls = vfs_android_get_tls();
	jmethodID m_getCacheDir = (*tls->jni)->GetMethodID(tls->jni, tls->activity.cls, "getCacheDir", "()Ljava/io/File;");
	jobject o_cacheFile = (*tls->jni)->CallObjectMethod(tls->jni, tls->activity.obj, m_getCacheDir);
	jobject c_cacheFile = (*tls->jni)->GetObjectClass(tls->jni, o_cacheFile);
	jmethodID m_getAbsolutePath = (*tls->jni)->GetMethodID(tls->jni, c_cacheFile, "getAbsolutePath", "()Ljava/lang/String;");
	jobject o_cachePath = (*tls->jni)->CallObjectMethod(tls->jni, o_cacheFile, m_getAbsolutePath);
	const char *jcachepath = (*tls->jni)->GetStringUTFChars(tls->jni, o_cachePath, 0);
	char *cachepath = strdup(jcachepath);
	(*tls->jni)->ReleaseStringUTFChars(tls->jni, o_cachePath, jcachepath);
	(*tls->jni)->DeleteLocalRef(tls->jni, o_cachePath);
	(*tls->jni)->DeleteLocalRef(tls->jni, o_cacheFile);
	(*tls->jni)->DeleteLocalRef(tls->jni, c_cacheFile);
	return cachepath;
}

void vfs_setup(CallChain next) {
	vfs_init();

	const char *storage_path = NULL;

	if(SDL_AndroidGetExternalStorageState() & (SDL_ANDROID_EXTERNAL_STORAGE_READ | SDL_ANDROID_EXTERNAL_STORAGE_WRITE)) {
		storage_path = SDL_AndroidGetExternalStoragePath();
	}

	if(!storage_path) {
		storage_path = SDL_AndroidGetInternalStoragePath();
	}

	assume(storage_path != NULL);

	char *cache_path = get_cache_path();
	assume(cache_path != NULL);

	char *local_res_path = strfmt("%s%cresources", storage_path, vfs_get_syspath_separator());
	vfs_syspath_normalize_inplace(local_res_path);

	log_info("Storage path: %s", storage_path);
	log_info("Local resource path: %s", local_res_path);
	log_info("Cache path: %s", cache_path);

	if(!vfs_mount_syspath("storage", storage_path, VFS_SYSPATH_MOUNT_MKDIR)) {
		log_fatal("Failed to mount '%s': %s", storage_path, vfs_get_error());
	}

	if(!vfs_mount_syspath("cache", cache_path, VFS_SYSPATH_MOUNT_MKDIR)) {
		log_fatal("Failed to mount '%s': %s", cache_path, vfs_get_error());
	}

	free(cache_path);

	vfs_create_union_mountpoint("resdirs");
	vfs_create_union_mountpoint("respkgs");
	vfs_create_union_mountpoint("res");

	if(!vfs_mount_assets("tmp", TAISEI_BUILDCONF_DATA_PATH)) {
		log_fatal("Failed to mount Android assets root: %s", vfs_get_error());
	}

	vfs_load_packages("tmp", "respkgs");
	vfs_mount_alias("resdirs", "tmp");
	vfs_unmount("tmp");

	if(!vfs_mount_syspath("tmp", local_res_path, VFS_SYSPATH_MOUNT_MKDIR | VFS_SYSPATH_MOUNT_READONLY)) {
		log_fatal("Failed to mount '%s': %s", local_res_path, vfs_get_error());
	}

	free(local_res_path);

	vfs_load_packages("tmp", "respkgs");
	vfs_mount_alias("resdirs", "tmp");
	vfs_unmount("tmp");

	vfs_mount_alias("res", "respkgs");
	vfs_mount_alias("res", "resdirs");
	// vfs_make_readonly("res");

	vfs_unmount("resdirs");
	vfs_unmount("respkgs");

	vfs_mkdir_required("storage/replays");
	vfs_mkdir_required("storage/screenshots");

	run_call_chain(&next, NULL);
}
