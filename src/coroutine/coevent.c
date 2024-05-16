/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "internal.h"

void coevent_init(CoEvent *evt) {
	static uint32_t g_uid;
	uint32_t uid = ++g_uid;
	EVT_DEBUG("Init event %p (uid = %u)", (void*)evt, uid);
	*evt = (CoEvent) { .unique_id = uid };
	assert(g_uid != 0);
}

CoEventSnapshot coevent_snapshot(const CoEvent *evt) {
	return (CoEventSnapshot) {
		.unique_id = evt->unique_id,
		.num_signaled = evt->num_signaled,
	};
}

CoEventStatus coevent_poll(const CoEvent *evt, const CoEventSnapshot *snap) {
#if 0
	EVT_DEBUG("[%p]", (void*)evt);
	EVT_DEBUG("evt->unique_id     == %u", evt->unique_id);
	EVT_DEBUG("snap->unique_id    == %u", snap->unique_id);
	EVT_DEBUG("evt->num_signaled  == %u", evt->num_signaled);
	EVT_DEBUG("snap->num_signaled == %u", snap->num_signaled);
#endif

	if(
		evt->unique_id != snap->unique_id ||
		evt->num_signaled < snap->num_signaled ||
		evt->unique_id == 0
	) {
		EVT_DEBUG("[%p / %u] Event was canceled", (void*)evt, evt->unique_id);
		return CO_EVENT_CANCELED;
	}

	if(evt->num_signaled > snap->num_signaled) {
		EVT_DEBUG("[%p / %u] Event was signaled", (void*)evt, evt->unique_id);
		return CO_EVENT_SIGNALED;
	}

	// EVT_DEBUG("Event hasn't changed; waiting...");
	return CO_EVENT_PENDING;
}

static bool subscribers_array_predicate(const void *pelem, void *userdata) {
	return cotask_unbox_notnull(*(const BoxedTask*)pelem);
}

void coevent_cleanup_subscribers(CoEvent *evt) {
	if(evt->subscribers.num_elements == 0) {
		return;
	}

	attr_unused uint prev_num_subs = evt->subscribers.num_elements;
	dynarray_filter(&evt->subscribers, subscribers_array_predicate, NULL);
	attr_unused uint new_num_subs = evt->subscribers.num_elements;
	EVT_DEBUG("Event %p num subscribers %u -> %u", (void*)evt, prev_num_subs, new_num_subs);
}

void coevent_add_subscriber(CoEvent *evt, CoTask *task) {
	EVT_DEBUG("Event %p (num=%u; capacity=%u)", (void*)evt, evt->subscribers.num_elements, evt->subscribers.capacity);
	EVT_DEBUG("Subscriber: %s", task->debug_label);

	*dynarray_append_with_min_capacity(&evt->subscribers, 4) = cotask_box(task);
}

static void coevent_wake_subscribers(CoEvent *evt, uint num_subs, BoxedTask subs[num_subs]) {
	for(int i = 0; i < num_subs; ++i) {
		CoTask *task = cotask_unbox_notnull(subs[i]);

		if(task && cotask_status(task) != CO_STATUS_DEAD) {
			EVT_DEBUG("Resume CoEvent{%p} subscriber %s", (void*)evt, task->debug_label);
			cotask_resume(task, NULL);
		}
	}
}

void coevent_signal(CoEvent *evt) {
	if(UNLIKELY(evt->unique_id == 0)) {
		return;
	}

	++evt->num_signaled;
	EVT_DEBUG("Signal event %p (uid = %u; num_signaled = %u)", (void*)evt, evt->unique_id, evt->num_signaled);
	assert(evt->num_signaled != 0);

	if(evt->subscribers.num_elements) {
		BoxedTask subs_snapshot[evt->subscribers.num_elements];
		memcpy(subs_snapshot, evt->subscribers.data, sizeof(subs_snapshot));
		evt->subscribers.num_elements = 0;
		coevent_wake_subscribers(evt, ARRAY_SIZE(subs_snapshot), subs_snapshot);
	}
}

void coevent_signal_once(CoEvent *evt) {
	if(!evt->num_signaled) {
		coevent_signal(evt);
	}
}

void coevent_cancel(CoEvent *evt) {
	TASK_DEBUG_EVENT(ev);

	if(evt->unique_id == 0) {
		EVT_DEBUG("[%lu] Event %p already canceled", ev, (void*)evt);
		return;
	}

	EVT_DEBUG("[%lu] BEGIN Cancel event %p (uid = %u; num_signaled = %u)", ev,  (void*)evt, evt->unique_id, evt->num_signaled);
	EVT_DEBUG("[%lu] SUBS = %p", ev,  (void*)evt->subscribers.data);
	evt->unique_id = 0;

	if(evt->subscribers.num_elements) {
		BoxedTask subs_snapshot[evt->subscribers.num_elements];
		memcpy(subs_snapshot, evt->subscribers.data, sizeof(subs_snapshot));
		dynarray_free_data(&evt->subscribers);
		coevent_wake_subscribers(evt, ARRAY_SIZE(subs_snapshot), subs_snapshot);
		// CAUTION: no modifying evt after this point, it may be invalidated
	} else {
		dynarray_free_data(&evt->subscribers);
	}

	EVT_DEBUG("[%lu] END Cancel event %p", ev, (void*)evt);
}

void _coevent_array_action(uint num, CoEvent *events, void (*func)(CoEvent*)) {
	for(uint i = 0; i < num; ++i) {
		func(events + i);
	}
}
