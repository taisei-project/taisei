/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "fbmgr.h"
#include "list.h"
#include "events.h"
#include "config.h"
#include "video.h"

typedef struct ManagedFramebufferData ManagedFramebufferData;

struct ManagedFramebufferData {
	LIST_INTERFACE(ManagedFramebufferData);
	List group_node;
	FramebufferResizeStrategy resize_strategy;
};

struct ManagedFramebufferGroup {
	List *members;
};

#define GET_MFB(mfb_data) CASTPTR_ASSUME_ALIGNED((char*)(mfb_data) - offsetof(ManagedFramebuffer, data), ManagedFramebuffer)
#define GET_DATA(mfb) CASTPTR_ASSUME_ALIGNED(&(mfb)->data, ManagedFramebufferData)
#define GROUPNODE_TO_DATA(gn) CASTPTR_ASSUME_ALIGNED((char*)(gn) - offsetof(ManagedFramebufferData, group_node), ManagedFramebufferData)

static ManagedFramebufferData *framebuffers;

static inline void fbmgr_framebuffer_get_metrics(ManagedFramebufferData *mfb_data, IntExtent *fb_size, FloatRect *fb_viewport) {
	assume(mfb_data->resize_strategy.resize_func != NULL);
	mfb_data->resize_strategy.resize_func(mfb_data->resize_strategy.userdata, fb_size, fb_viewport);
}

static void fbmgr_framebuffer_update(ManagedFramebufferData *mfb_data) {
	IntExtent fb_size;
	FloatRect fb_viewport;
	Framebuffer *fb = GET_MFB(mfb_data)->fb;

	if(mfb_data->resize_strategy.resize_func == NULL) {
		return;
	}

	fbmgr_framebuffer_get_metrics(mfb_data, &fb_size, &fb_viewport);

	for(uint i = 0; i < FRAMEBUFFER_MAX_ATTACHMENTS; ++i) {
		fbutil_resize_attachment(fb, i, fb_size.w, fb_size.h);
	}

	r_framebuffer_viewport_rect(fb, fb_viewport);
}

static void fbmgr_framebuffer_update_all(void) {
	for(ManagedFramebufferData *d = framebuffers; d; d = d->next) {
		fbmgr_framebuffer_update(d);
	}
}

ManagedFramebuffer *fbmgr_framebuffer_create(const char *name, const FramebufferConfig *cfg) {
	assert(cfg->attachments != NULL);
	assert(cfg->num_attachments >= 1);

	auto mfb = ALLOC_FLEX(ManagedFramebuffer, sizeof(ManagedFramebufferData));
	ManagedFramebufferData *data = GET_DATA(mfb);
	data->resize_strategy = cfg->resize_strategy;
	mfb->fb = r_framebuffer_create();
	r_framebuffer_set_debug_label(mfb->fb, name);

	FloatRect fb_viewport;

	if(data->resize_strategy.resize_func != NULL) {
		FBAttachmentConfig ac[cfg->num_attachments];
		memcpy(ac, cfg->attachments, sizeof(ac));

		IntExtent fb_size;

		fbmgr_framebuffer_get_metrics(data, &fb_size, &fb_viewport);

		for(int i = 0; i < cfg->num_attachments; ++i) {
			ac[i].tex_params.width = fb_size.w;
			ac[i].tex_params.height = fb_size.h;
		}

		fbutil_create_attachments(mfb->fb, ARRAY_SIZE(ac), ac);
	} else {
		FBAttachmentConfig *ac = cfg->attachments;
		fb_viewport = (FloatRect) {
			.extent = { ac[0].tex_params.width, ac[0].tex_params.height }
		};
		fbutil_create_attachments(mfb->fb, cfg->num_attachments, cfg->attachments);
	}

	r_framebuffer_viewport_rect(mfb->fb, fb_viewport);
	r_framebuffer_clear(mfb->fb, BUFFER_ALL, RGBA(0, 0, 0, 0), 1);

	list_push(&framebuffers, data);
	return mfb;
}

Framebuffer *fbmgr_framebuffer_disown(ManagedFramebuffer *mfb) {
	for(ManagedFramebufferData *d = framebuffers; d; d = d->next) {
		ManagedFramebuffer *m = GET_MFB(d);

		if(m == mfb) {
			if(d->resize_strategy.cleanup_func) {
				d->resize_strategy.cleanup_func(d->resize_strategy.userdata);
			}

			Framebuffer *fb = m->fb;
			list_unlink(&framebuffers, d);
			mem_free(m);
			return fb;
		}
	}

	UNREACHABLE;
}

void fbmgr_framebuffer_destroy(ManagedFramebuffer *mfb) {
	Framebuffer *fb = fbmgr_framebuffer_disown(mfb);
	fbutil_destroy_attachments(fb);
	r_framebuffer_destroy(fb);
}

static bool fbmgr_event(SDL_Event *evt, void *arg) {
	switch(TAISEI_EVENT(evt->type)) {
		case TE_VIDEO_MODE_CHANGED:
			fbmgr_framebuffer_update_all();
			break;

		case TE_CONFIG_UPDATED:
			switch(evt->user.code) {
				case CONFIG_POSTPROCESS:
				case CONFIG_BG_QUALITY:
				case CONFIG_FG_QUALITY:
					fbmgr_framebuffer_update_all();
					break;

				default: break;
			}

			break;
	}

	return false;
}

void fbmgr_init(void) {
	events_register_handler(&(EventHandler) {
		fbmgr_event, NULL, EPRIO_SYSTEM,
	});
}

void fbmgr_shutdown(void) {
	events_unregister_handler(fbmgr_event);
}

ManagedFramebufferGroup *fbmgr_group_create(void) {
	return ALLOC(ManagedFramebufferGroup);
}

void fbmgr_group_destroy(ManagedFramebufferGroup *group) {
	for(List *n = group->members, *next; n; n = next) {
		next = n->next;
		ManagedFramebuffer *mfb = GET_MFB(GROUPNODE_TO_DATA(n));
		fbmgr_framebuffer_destroy(mfb);
	}

	mem_free(group);
}

Framebuffer *fbmgr_group_framebuffer_create(ManagedFramebufferGroup *group, const char *name, const FramebufferConfig *cfg) {
	ManagedFramebuffer *mfb = fbmgr_framebuffer_create(name, cfg);
	ManagedFramebufferData *mfb_data = GET_DATA(mfb);
	list_push(&group->members, &mfb_data->group_node);
	return mfb->fb;
}

void fbmgr_group_fbpair_create(ManagedFramebufferGroup *group, const char *name, const FramebufferConfig *cfg, FBPair *fbpair) {
	char buf[R_DEBUG_LABEL_SIZE];
	snprintf(buf, sizeof(buf), "%s FB 1", name);
	fbpair->front = fbmgr_group_framebuffer_create(group, buf, cfg);
	snprintf(buf, sizeof(buf), "%s FB 2", name);
	fbpair->back = fbmgr_group_framebuffer_create(group, buf, cfg);
}

void fbmgr_resize_strategy_screensized(void *ignored, IntExtent *out_dimensions, FloatRect *out_viewport) {
	float w, h;
	video_get_viewport_size(&w, &h);
	*out_dimensions = (IntExtent) { w, h };
	*out_viewport = (FloatRect) { 0, 0, w, h };
}
