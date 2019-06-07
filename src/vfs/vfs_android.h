/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef IGUARD_vfs_vfs_android_h
#define IGUARD_vfs_vfs_android_h

#include "taisei.h"

#include <SDL.h>
#include <jni.h>

typedef struct VFSAndroidTLS {
	JNIEnv *jni;

	struct {
		jobject obj;
		jclass cls;

		struct {
			jmethodID queryAssetPath;
			jmethodID getAssets;
		} methods;
	} activity;

	struct {
		jobject obj;
		jclass cls;

		struct {
			jmethodID list;
		} methods;
	} assetManager;
} VFSAndroidTLS;

VFSAndroidTLS *vfs_android_get_tls(void);
void vfs_android_make_ref_global(jobject *ref);

#endif // IGUARD_vfs_vfs_android_h
