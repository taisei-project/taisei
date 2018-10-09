
#include "taisei.h"

#include "hashtable.h"
#include "list.h"
#include "util/stringops.h"
#include "util/assert.h"
#include "log.h"

#include <stdlib.h>
#include <SDL.h>

/******************************************************************************\
 *                                      ,.                                    *
 *    ```             `.,:::,.    `;'';,;'.`                                  *
 *   `,`,;'''''';.+###############''''''' ,.`                                 *
 *  ,.''.:''''+@@####################'';,' `:                                 *
 *  ,`',;'''+#@@#########################++;,                                 *
 *   :,';+++@@#############################:,:                                *
 *  .::,;++@@###############################;                                 *
 *  .,:+;+,,,:+##@##########@##############+:,                                *
 *   +,:+#@@##'::::++#######@###@##@##+'::,;'#`                               *
 *   @++:@@@@@#@@@@###@#@##@##'@@##@@##@@@##@##                               *
 *  .+'++@@@@##@@@@#@#@@#+#@@#:'@##@@@@@#@#@@##                               *
 * .,+;+'+@@@##@@@@@+@+@@;';@@.`:#@;';@##@#@#@@#,                             *
 * :#:+'+@@@#@@@@@:+;:@::':@,``.:@,+::@:@#@@#@#;                              *
 *  '::+'@@@@@@@@#`,+`++';.`````:``+++,+:@@@.;@;                              *
 *  +@'+;@@@@#:,;;,`, ''';`````````+''+:.';.: '`                              *
 *  +@@@@@@@#:+',:`.,,.````````````.,,,.,:''.                                 *
 *  '@@@@@@@':'+'+`....```````````````...:'+   Type-safe generics? In my C?   *
 *  ;@@@@@@@':'+''`````````.`````````````,'+                                  *
 *  :#@@@@@@@:'+''`````````::::''````````:;; It's more likely than you think. *
 *  .;@@@@@@@@++,'`````````'::::.````````;:`:                                 *
 *   .@@#@@@@#;'.:.`````````':'``````````@.:                                  *
 *   ,#@,@@;@##@##``````````````````````@@#,        [FREE HEADER CHECK]       *
 *   :.@ +# @##@@# :``````````````````;@:@@:                                  *
 *     # '``.@:,##   :`````````````.+@#,..@:                                  *
 *       `   '`  @      `::,,,:::` ` +; . ;'                                  *
 *                :                        +                                  *
\******************************************************************************/

/***********************\
 * User-defined macros *
\***********************/

/*
 * HT_SUFFIX
 *
 * A name tag associated with your custom hashtable type.
 * You must define this to a valid, unique identifier.
 * Whenever you see XXX appear as part of an identifier in the documentation,
 * substitute it for the value of HT_SUFFIX.
 *
 * Example:
 *
 *         #define HT_SUFFIX str2ptr
 */
#ifndef HT_SUFFIX
	#error HT_SUFFIX not defined
#endif

/*
 * HT_KEY_TYPE
 *
 * The type of hashtable keys. This may be virtually anything, except for
 * incomplete types. It is possible to use a pointer to a heap-allocated
 * data structure here that is to be used as the actual key (see below). This
 * is useful if you want to use strings, for example.
 *
 * Example:
 *
 *         #define HT_KEY_TYPE char*
 */
#ifndef HT_KEY_TYPE
	#error HT_KEY_TYPE not defined
#endif

/*
 * HT_KEY_CONST
 *
 * Optional.
 *
 * If defined, the 'const' qualifier will be prepended to the key type where
 * appropriate. Useful for pointer types.
 */
#ifdef HT_KEY_CONST
	#undef HT_KEY_CONST
	#define HT_KEY_CONST const
#else
	#define HT_KEY_CONST
#endif

/*
 * HT_KEY_TYPE
 *
 * The type of hashtable keys. This must be a complete, non-array type.
 *
 * Example:
 *
 *         #define HT_VALUE_TYPE void*
 */
#ifndef HT_VALUE_TYPE
	#error HT_VALUE_TYPE not defined
#endif

/*
 * HT_FUNC_HASH_KEY(key)
 *
 * A function-like macro used to compute an integer hash based on key. It must
 * produce consistent results for every possible key, and collisions should be
 * minimized to improve performance.
 *
 * Type of the key parameter is the same as HT_KEY_TYPE.
 * Type of the result expression should be hash_t.
 *
 * Example:
 *
 *         #define HT_FUNC_HASH_KEY(key) htutil_hashfunc_string(key)
 */
#ifndef HT_FUNC_HASH_KEY
	#error HT_FUNC_HASH_KEY not defined
#endif

/*
 * HT_FUNC_KEYS_EQUAL(key1, key2)
 *
 * Optional.
 *
 * A function-like macro used to test two keys for equality. Note that if two
 * keys are considered equal, they must also hash to the same value. However,
 * the converse doesn't apply: equality of hashes doesn't imply equality of keys.
 *
 * Type of the key1 and key2 parameters is the same as HT_KEY_TYPE.
 * The result expression must be true if the keys are equal, false otherwise.
 *
 * If no definition is provided, the standard == operator is used.
 *
 * Example:
 *
 *         #define HT_FUNC_KEYS_EQUAL(key1, key2) (!strcmp(key1, key2))
 */
#ifndef HT_FUNC_KEYS_EQUAL
	#define HT_FUNC_KEYS_EQUAL(key1, key2) ((key1) == (key2))
#endif

/*
 * HT_FUNC_COPY_KEY(dst, src)
 *
 * Optional.
 *
 * A function-like macro that specifies how copying a key should be handled.
 * This is useful with pointers to variably-sized structures, such as strings.
 *
 * Type of src is the same as HT_KEY_TYPE.
 * dst is a pointer to HT_KEY_TYPE.
 * Type of the result expression is ignored.
 *
 * If no definition is provided, the key is simply copied by value.
 *
 * Example:
 *
 *        #define HT_FUNC_COPY_KEY(dst, src) (*(dst) = strdup(src))
 */
#ifndef HT_FUNC_COPY_KEY
	#define HT_FUNC_COPY_KEY(dst, src) (*(dst) = (src))
#endif

/*
 * HT_FUNC_FREE_KEY(key)
 *
 * Optional.
 *
 * A function-like macro used to free any resources allocated for a key, if any.
 * Use this to clean up after after any allocations done by HT_FUNC_COPY_KEY.
 *
 * Example:
 *
 *        #define HT_FUNC_FREE_KEY(key) free(key)
 */
#ifndef HT_FUNC_FREE_KEY
	#define HT_FUNC_FREE_KEY(key) ((void)(key))
#endif

/*
 * HT_THREAD_SAFE
 *
 * Optional.
 *
 * If defined, most hashtable operations will be guarded by a readers-writer lock,
 * protecting them from data races when used by multiple threads. This has some
 * impact on performance and memory usage, however.
 *
 * Some additional APIs are provided in this mode, as well as unsafe versions of
 * some of the core APIs. They are documented below.
 *
 * The unsafe variants behave identically to their core counterparts, but they
 * completely bypass the locking mechanism, making them somewhat faster.
 *
 * Example:
 *
 *        #define HT_THREAD_SAFE
 */
#ifndef HT_THREAD_SAFE
	// no default needed
#endif

/*
 * HT_DECL, HT_IMPL
 *
 * You must define at least one of these macros.
 *
 * If HT_DECL is defined, this header will generate declarations of the API types
 * and functions. This should be used in headers.
 *
 * If HT_IMPL is defined, then HT_DECL is implied, and definitions of the API
 * functions are also generated.
 *
 *  * Example:
 *
 *        #define HT_IMPL
 *        #include "hashtable.inc.h"
 */
#if !defined(HT_DECL) && !defined(HT_IMPL)
	#error neither HT_DECL nor HT_IMPL defined
#elif defined(HT_IMPL) && !defined(HT_DECL)
	#define HT_DECL
#endif

/*
 * HT_MIN_SIZE
 *
 * Optional.
 *
 * How many buckets to allocate initially. Higher values increase memory usage,
 * but may improve performance. Must be a power of 2.
 */
#ifndef HT_MIN_SIZE
	#define HT_MIN_SIZE 32
#endif

// TODO: static_assert HT_MIN_SIZE is a power of 2

/*
 * The following macros comprise the core of the templating machinery.
 * They are used to construct identifiers augmented with HT_SUFFIX.
 */

#define _HT_NAME_INNER1(suffix, name) ht_##suffix##name
#define _HT_NAME_INNER2(suffix, name) _HT_NAME_INNER1(suffix, name)
#define HT_NAME(name) _HT_NAME_INNER2(HT_SUFFIX, name)

#define _HT_PRIV_NAME_INNER1(suffix, name) _ht_##suffix##name
#define _HT_PRIV_NAME_INNER2(suffix, name) _HT_PRIV_NAME_INNER1(suffix, name)
#define HT_PRIV_NAME(name) _HT_PRIV_NAME_INNER2(HT_SUFFIX, name)

#define HT_TYPE(name) HT_NAME(_##name##_t)
#define HT_FUNC(name) HT_NAME(_##name)
#define HT_PRIV_FUNC(name) HT_PRIV_NAME(_##name)
#define HT_BASETYPE HT_NAME(_t)

#define HT_DECLARE_FUNC(return_type, name, arguments) return_type HT_FUNC(name) arguments
#define HT_DECLARE_PRIV_FUNC(return_type, name, arguments) static return_type HT_PRIV_FUNC(name) arguments
#define HT_INLINE inline attr_must_inline

/****************\
 * Declarations *
\****************/
#ifdef HT_DECL

/*
 * ht_XXX_t
 *
 * A structure representing a hash table.
 * All its fields are considered private and should be left unmolested,
 * please use the API functiosn instead of manipulating these directly.
 */
typedef struct HT_BASETYPE HT_BASETYPE;

/*
 * ht_XXX_key_t
 *
 * An alias for the key type (HT_KEY_TYPE).
 */
typedef HT_KEY_TYPE HT_TYPE(key);

/*
 * ht_XXX_const_key_t
 *
 * An alias for the key type (HT_KEY_TYPE).
 *
 * If HT_KEY_CONST is defined, this type has the 'const' qualifier.
 */
typedef HT_KEY_CONST HT_KEY_TYPE HT_TYPE(const_key);

/*
 * ht_XXX_value_t
 *
 * An alias for the value type (HT_VALUE_TYPE).
 */
typedef HT_VALUE_TYPE HT_TYPE(value);

/*
 * ht_XXX_key_list_t
 *
 * A list of keys.
 */
typedef struct HT_TYPE(key_list) HT_TYPE(key_list);

/*
 * ht_XXX_foreach_callback_t
 *
 * Pointer to a callback function for use with ht_XXX_foreach().
 */
typedef void* (*HT_TYPE(foreach_callback))(HT_TYPE(const_key) key, HT_TYPE(value) data, void *arg);

/*
 * ht_XXX_iter_t
 *
 * An iterator structure. See ht_XXX_iter_begin().
 */
typedef struct HT_TYPE(iter) HT_TYPE(iter);

/*
 * Forward declaration of the private element struct.
 */
typedef struct HT_TYPE(element) HT_TYPE(element);

/*
 * Definition for ht_XXX_key_list_t.
 */
struct HT_TYPE(key_list) {
	LIST_INTERFACE(HT_TYPE(key_list));
	HT_TYPE(key) key;
};

/*
 * Definition for ht_XXX_t.
 * All of these fields are to be considered private.
 */
struct HT_BASETYPE {
	HT_TYPE(element) **table;
	size_t num_elements;
	size_t table_size;
	size_t hash_mask;

#ifdef HT_THREAD_SAFE
	struct {
		SDL_mutex *mutex;
		SDL_cond *cond;
		uint readers;
		bool writing;
	} sync;
#endif
};

/*
 * Definition for ht_XXX_iter_t.
 */
struct HT_TYPE(iter) {
	HT_BASETYPE *hashtable;
	HT_TYPE(key) key;
	HT_TYPE(value) value;
	bool has_data;

	struct {
		size_t bucketnum;
		HT_TYPE(element) *elem;
	} private;
};

/*
 * void ht_XXX_create(ht_XXX_t *ht);
 *
 * Initialize a hashtable structure. Must be called before any other API function.
 */
HT_DECLARE_FUNC(void, create, (HT_BASETYPE *ht))
	attr_nonnull(1);

/*
 * void ht_XXX_destroy(ht_XXX_t *ht);
 *
 * Destroy a hashtable, freeing all resources allocated to it.
 * Other API functions, except for ht_XXX_create(), must not be called after this.
 * Does not free the memory pointed to by [ht], as it's not necessarily heap-allocated.
 */
HT_DECLARE_FUNC(void, destroy, (HT_BASETYPE *ht))
	attr_nonnull(1);

/*
 * ht_XXX_t* ht_XXX_new(void);
 *
 * Convenience function; allocates and initializes a new hashtable structure.
 * You must free() it manually when you're done with it (but don't forget to
 * ht_*_destroy() it as well).
 *
 * Returns the allocated hashtable structure.
 */
static HT_INLINE attr_returns_nonnull attr_nodiscard
HT_DECLARE_FUNC(HT_BASETYPE*, new, (void))  {
	HT_BASETYPE *ht = calloc(1, sizeof(HT_BASETYPE));
	HT_FUNC(create)(ht);
	return ht;
}

#ifdef HT_THREAD_SAFE

/*
 * void ht_XXX_lock(ht_XXX_t *ht);
 *
 * Acquire a read lock on a hashtable. The hashtable is guarateed to stay unmodified
 * while this lock is held. You must not try to modify it from the same thread; that
 * will result in a deadlock. Other threads are able to read the hashtable while the
 * lock is held.
 */
HT_DECLARE_FUNC(void, lock, (HT_BASETYPE *ht))
	attr_nonnull(1);

/*
 * void ht_XXX_unlock(ht_XXX_t *ht);
 *
 * Release a read lock on a hashtable previously acquired via ht_XXX_unlock().
 */
HT_DECLARE_FUNC(void, unlock, (HT_BASETYPE *ht))
	attr_nonnull(1);

#endif // HT_THREAD_SAFE

/*
 * ht_XXX_value_t ht_XXX_get(ht_XXX_t *ht, ht_XXX_const_key_t key, ht_XXX_const_value_t fallback);
 *
 * Retrieve a value associated with [key]. If there is no association, [fallback] will
 * be returned instead.
 */
HT_DECLARE_FUNC(HT_TYPE(value), get, (HT_BASETYPE *ht, HT_TYPE(const_key) key, HT_TYPE(value) fallback))
	attr_nonnull(1);

#ifdef HT_THREAD_SAFE
/*
 * ht_XXX_value_t ht_XXX_get_unsafe(ht_XXX_t *ht, ht_XXX_const_key_t key, ht_XXX_const_value_t fallback);
 *
 * A non-thread-safe version of ht_XXX_get().
 */
HT_DECLARE_FUNC(HT_TYPE(value), get_unsafe, (HT_BASETYPE *ht, HT_TYPE(const_key) key, HT_TYPE(value) fallback))
	attr_nonnull(1);
#endif // HT_THREAD_SAFE

/*
 * bool ht_XXX_lookup(ht_XXX_t *ht, ht_XXX_const_key_t key, ht_XXX_value_t *out_value);
 *
 * Check whether a key exists in the hashtable. If it does and [out_value] is not NULL,
 * then the value associated with it will be also copied into *out_value.
 *
 * Returns true if an entry is found, false otherwise.
 */
HT_DECLARE_FUNC(bool, lookup, (HT_BASETYPE *ht, HT_TYPE(const_key) key, HT_TYPE(value) *out_value))
	attr_nonnull(1) attr_nodiscard;

#ifdef HT_THREAD_SAFE
/*
 * bool ht_XXX_lookup_unsafe(ht_XXX_t *ht, ht_XXX_const_key_t key, ht_XXX_value_t *out_value);
 *
 * A non-thread-safe version of ht_XXX_lookup().
 */
HT_DECLARE_FUNC(bool, lookup_unsafe, (HT_BASETYPE *ht, HT_TYPE(const_key) key, HT_TYPE(value) *out_value))
	attr_nonnull(1) attr_nodiscard;
#endif // HT_THREAD_SAFE

/*
 * void ht_XXX_set(ht_XXX_t *ht, ht_XXX_const_key_t key, ht_XXX_const_value_t value);
 *
 * Store a key-value pair in the hashtable.
 * If this key already has a value associated with it, it will be overwritten.
 *
 * Returns true if a new key was inserted, false if a previous value was overwritten.
 */
HT_DECLARE_FUNC(bool, set, (HT_BASETYPE *ht, HT_TYPE(const_key) key, HT_TYPE(value) value))
	attr_nonnull(1);

/*
 * bool ht_XXX_try_set(ht_XXX_t ht, ht_XXX_const_key_t key, ht_XXX_const_value_t value, ht_XXX_value_t (*value_transform)(ht_XXX_value_t), ht_XXX_value_t *out_value);
 *
 * See if [key] exists in the hashtable, and then:
 *
 * 	if key exists, then:
 * 		if out_value is not NULL, then:
 * 			store value associated with key in *out_value;
 *
 * 		return false;
 * 	else:
 * 		if value_transform is not NULL, then:
 * 			newValue = value_transform(value);
 * 		else
 * 			newValue = value;
 *
 * 		if out_value is not NULL, then:
 * 			store newValue in *out_value;
 *
 * 		associate key with newValue;
 *
 * 		return true;
 *
 * With HT_THREAD_SAFE defined, this is an atomic operation: the algorithm holds
 * a write lock for its whole duration.
 */
HT_DECLARE_FUNC(bool, try_set, (HT_BASETYPE *ht, HT_TYPE(const_key) key, HT_TYPE(value) value, HT_TYPE(value) (*value_transform)(HT_TYPE(value)), HT_TYPE(value) *out_value))
	attr_nonnull(1) attr_nodiscard;

/*
 * bool ht_XXX_unset(ht_XXX_t *ht, ht_XXX_const_key_t key);
 *
 * If there's a value associated with key, remove that association and return true.
 * Otherwise, return false.
 */
HT_DECLARE_FUNC(bool, unset, (HT_BASETYPE *ht, HT_TYPE(const_key) key))
	attr_nonnull(1);

/*
 * void ht_XXX_unset_list(ht_XXX_t *ht, const ht_XXX_key_list_t *keylist);
 *
 * Functionally equality to calling ht_XXX_unset() for every key in keylist, but
 * more efficient.
 */
HT_DECLARE_FUNC(void, unset_list, (HT_BASETYPE *ht, const HT_TYPE(key_list) *key_list))
	attr_nonnull(1);

/*
 * void ht_XXX_unset_all(ht_XXX_t *ht);
 *
 * Empty the hashtable completely.
 * Functionally equivalent to calling ht_XXX_unset() for every key in the table,
 * but more efficient.
 */
HT_DECLARE_FUNC(void, unset_all, (HT_BASETYPE *ht))
	attr_nonnull(1);

/*
 * void* ht_XXX_foreach(ht_XXX_t *ht, ht_XXX_callback_t callback, void *arg);
 *
 * Call callback(key, value, arg) for each key-value pair in the hashtable.
 *
 * If the callback returns anything other than a NULL pointer, the loop is broken
 * early, and this function returns whatever the callback returned.
 *
 * Otherwise, this function returns NULL once every pair has been processed.
 *
 * WARNING: Do not try to modify the hashtable from inside the callback.
 */
HT_DECLARE_FUNC(void*, foreach, (HT_BASETYPE *ht, HT_TYPE(foreach_callback) callback, void *arg))
	attr_nonnull(1, 2);

/*
 * void ht_XXX_iter_begin(ht_XXX_t *ht, ht_XXX_iter_t *iter);
 * void ht_XXX_iter_next(ht_XXX_iter_t *iter);
 * void ht_XXX_iter_end(ht_XXX_iter_t *iter);
 *
 * These functions are used to iterate over the key-value pairs stored in the
 * hashtable without having to provide a callback function.
 *
 * Declare a ht_XXX_iter_t structure and use ht_XXX_iter_begin() to initialize it.
 * Then, if iter->has_data is true, iter->key and iter->value are set to the key
 * and value of the first pair in the hashtable, respectively.
 *
 * While iter->has_data is true, you can call ht_XXX_iter_next() to advance the
 * iterator to the next pair. If there are no more pairs, ht_XXX_iter_next() sets
 * iter->has_data to false. Otherwise, iter->key and iter->data are updated.
 * Calling ht_XXX_iter_next() has no effect if iter->has_data is already false.
 *
 * You must call ht_XXX_iter_end() when you are done iterating.
 *
 * Example:
 *
 * 		ht_XXX_iter_t iter;
 * 		ht_XXX_iter_begin(&my_hashtable, &iter);
 *
 * 		while(iter.has_data) {
 * 			do_something_with(iter.key, iter.value);
 *
 * 			if(some_condition) {
 * 				break;
 * 			}
 *
 * 			ht_XXX_iter_next(&iter);
 * 		}
 *
 * 		ht_XXX_iter_end(&iter);
 *
 * WARNING: Do not try to modify the hashtable while iterating over its contents.
 *
 * If HT_THREAD_SAFE is defined, ht_XXX_iter_begin() acquires a read lock, and
 * ht_XXX_iter_end() releases it. Thus, the hashtable is guarateed to stay
 * unmodified while any threads have an iterator active.
 */
HT_DECLARE_FUNC(void, iter_begin, (HT_BASETYPE *ht, HT_TYPE(iter) *iter))
	attr_nonnull(1, 2);

HT_DECLARE_FUNC(void, iter_next, (HT_TYPE(iter) *iter))
	attr_nonnull(1);

HT_DECLARE_FUNC(void, iter_end, (HT_TYPE(iter) *iter))
	attr_nonnull(1);

#endif // HT_DECL

/*******************\
 * Implementations *
\*******************/
#ifdef HT_IMPL

struct HT_TYPE(element) {
	LIST_INTERFACE(HT_TYPE(element));
	HT_TYPE(key) key;
	HT_TYPE(value) value;
	hash_t hash;
};

HT_DECLARE_PRIV_FUNC(void, begin_write, (HT_BASETYPE *ht)) {
	#ifdef HT_THREAD_SAFE
	SDL_LockMutex(ht->sync.mutex);

	while(ht->sync.writing || ht->sync.readers) {
		SDL_CondWait(ht->sync.cond, ht->sync.mutex);
	}

	ht->sync.writing = true;
	SDL_UnlockMutex(ht->sync.mutex);
	#endif
}

HT_DECLARE_PRIV_FUNC(void, end_write, (HT_BASETYPE *ht)) {
	#ifdef HT_THREAD_SAFE
	SDL_LockMutex(ht->sync.mutex);
	ht->sync.writing = false;
	SDL_CondBroadcast(ht->sync.cond);
	SDL_UnlockMutex(ht->sync.mutex);
	#endif
}

HT_DECLARE_PRIV_FUNC(void, begin_read, (HT_BASETYPE *ht)) {
	#ifdef HT_THREAD_SAFE
	SDL_LockMutex(ht->sync.mutex);

	while(ht->sync.writing) {
		SDL_CondWait(ht->sync.cond, ht->sync.mutex);
	}

	++ht->sync.readers;
	SDL_UnlockMutex(ht->sync.mutex);
	#endif
}

HT_DECLARE_PRIV_FUNC(void, end_read, (HT_BASETYPE *ht)) {
	#ifdef HT_THREAD_SAFE
	SDL_LockMutex(ht->sync.mutex);

	if(!--ht->sync.readers) {
		SDL_CondSignal(ht->sync.cond);
	}

	SDL_UnlockMutex(ht->sync.mutex);
	#endif
}

#ifdef HT_THREAD_SAFE
HT_DECLARE_FUNC(void, lock, (HT_BASETYPE *ht)) {
	HT_PRIV_FUNC(begin_read)(ht);
}

HT_DECLARE_FUNC(void, unlock, (HT_BASETYPE *ht)) {
	HT_PRIV_FUNC(end_read)(ht);
}
#endif // HT_THREAD_SAFE

HT_DECLARE_FUNC(void, create, (HT_BASETYPE *ht)) {
	size_t size = HT_MIN_SIZE;

	ht->table = calloc(size, sizeof(HT_TYPE(element) *));
	ht->table_size = size;
	ht->hash_mask = size - 1;
	ht->num_elements = 0;

	#ifdef HT_THREAD_SAFE
	ht->sync.writing = false;
	ht->sync.readers = 0;
	ht->sync.mutex = SDL_CreateMutex();
	ht->sync.cond = SDL_CreateCond();
	#endif
}

HT_DECLARE_FUNC(void, destroy, (HT_BASETYPE *ht)) {
	HT_FUNC(unset_all)(ht);
	#ifdef HT_THREAD_SAFE
	SDL_DestroyCond(ht->sync.cond);
	SDL_DestroyMutex(ht->sync.mutex);
	#endif
	free(ht->table);
}

HT_DECLARE_PRIV_FUNC(HT_TYPE(element)*, find_element, (HT_BASETYPE *ht, HT_TYPE(const_key) key, hash_t hash)) {
	HT_TYPE(element) *elems = ht->table[hash & ht->hash_mask];

	for(HT_TYPE(element) *e = elems; e; e = e->next) {
		if(hash == e->hash && HT_FUNC_KEYS_EQUAL(key, e->key)) {
			return e;
		}
	}

	return NULL;
}

HT_DECLARE_FUNC(HT_TYPE(value), get, (HT_BASETYPE *ht, HT_TYPE(const_key) key, HT_TYPE(value) fallback)) {
	hash_t hash = HT_FUNC_HASH_KEY(key);
	HT_TYPE(element) *elem;
	HT_TYPE(value) value;

	HT_PRIV_FUNC(begin_read)(ht);
	elem = HT_PRIV_FUNC(find_element)(ht, key, hash);
	value = elem ? elem->value : fallback;
	HT_PRIV_FUNC(end_read)(ht);

	return value;
}

#ifdef HT_THREAD_SAFE
HT_DECLARE_FUNC(HT_TYPE(value), get_unsafe, (HT_BASETYPE *ht, HT_TYPE(const_key) key, HT_TYPE(value) fallback)) {
	hash_t hash = HT_FUNC_HASH_KEY(key);
	HT_TYPE(element) *elem = HT_PRIV_FUNC(find_element)(ht, key, hash);
	return elem ? elem->value : fallback;
}
#endif // HT_THREAD_SAFE

HT_DECLARE_FUNC(bool, lookup, (HT_BASETYPE *ht, HT_TYPE(const_key) key, HT_TYPE(value) *out_value)) {
	hash_t hash = HT_FUNC_HASH_KEY(key);
	HT_TYPE(element) *elem;
	bool found = false;

	HT_PRIV_FUNC(begin_read)(ht);
	elem = HT_PRIV_FUNC(find_element)(ht, key, hash);

	if(elem != NULL) {
		if(out_value != NULL) {
			*out_value = elem->value;
		}

		found = true;
	}

	HT_PRIV_FUNC(end_read)(ht);

	return found;
}

#ifdef HT_THREAD_SAFE
HT_DECLARE_FUNC(bool, lookup_unsafe, (HT_BASETYPE *ht, HT_TYPE(const_key) key, HT_TYPE(value) *out_value)) {
	hash_t hash = HT_FUNC_HASH_KEY(key);
	HT_TYPE(element) *elem = HT_PRIV_FUNC(find_element)(ht, key, hash);

	if(elem != NULL) {
		if(out_value != NULL) {
			*out_value = elem->value;
		}

		return true;
	}

	return false;
}
#endif // HT_THREAD_SAFE

HT_DECLARE_PRIV_FUNC(void*, delete_callback, (List **vlist, List *velem, void *vht)) {
	HT_TYPE(element) *elem = (HT_TYPE(element) *) velem;
	HT_FUNC_FREE_KEY(elem->key);
	free(list_unlink(vlist, velem));
	return NULL;
}

HT_DECLARE_PRIV_FUNC(void, unset_all, (HT_BASETYPE *ht)) {
	for(size_t i = 0; i < ht->table_size; ++i) {
		list_foreach((ht->table + i), HT_PRIV_FUNC(delete_callback), ht);
	}

	ht->num_elements = 0;
}

HT_DECLARE_FUNC(void, unset_all, (HT_BASETYPE *ht)) {
	HT_PRIV_FUNC(begin_write)(ht);
	HT_PRIV_FUNC(unset_all)(ht);
	HT_PRIV_FUNC(end_write)(ht);
}

HT_DECLARE_FUNC(bool, unset, (HT_BASETYPE *ht, HT_TYPE(const_key) key)) {
	HT_TYPE(element) *elem;
	hash_t hash = HT_FUNC_HASH_KEY(key);
	bool success = false;

	HT_PRIV_FUNC(begin_write)(ht);
	elem = HT_PRIV_FUNC(find_element)(ht, key, hash);

	if(elem) {
		HT_TYPE(element) **elist = ht->table + (hash & ht->hash_mask);
		HT_FUNC_FREE_KEY(elem->key);
		free(list_unlink(elist, elem));
		--ht->num_elements;
		success = true;
	}
	HT_PRIV_FUNC(end_write)(ht);

	return success;
}

HT_DECLARE_FUNC(void, unset_list, (HT_BASETYPE *ht, const HT_TYPE(key_list) *key_list)) {
	HT_PRIV_FUNC(begin_write)(ht);

	for(const HT_TYPE(key_list) *i = key_list; i; i = i->next) {
		hash_t hash = HT_FUNC_HASH_KEY(i->key);
		HT_TYPE(element) *elem = HT_PRIV_FUNC(find_element)(ht, i->key, hash);

		if(elem) {
			HT_TYPE(element) **elist = ht->table + (hash & ht->hash_mask);
			HT_FUNC_FREE_KEY(elem->key);
			free(list_unlink(elist, elem));
			--ht->num_elements;
		}
	}

	HT_PRIV_FUNC(end_write)(ht);
}

HT_DECLARE_PRIV_FUNC(bool, set, (
	HT_BASETYPE *ht,
	HT_TYPE(element) **table,
	size_t hash_mask,
	hash_t hash,
	HT_TYPE(const_key) key,
	HT_TYPE(value) value,
	HT_TYPE(value) (*transform_value)(HT_TYPE(value)),
	bool allow_overwrite,
	HT_TYPE(value) *out_value
)) {
	size_t idx = hash & hash_mask;
	HT_TYPE(element) **elems = table + idx, *elem = NULL;

	for(HT_TYPE(element) *e = *elems; e; e = e->next) {
		if(hash == e->hash && HT_FUNC_KEYS_EQUAL(key, e->key)) {
			if(!allow_overwrite) {
				if(out_value != NULL) {
					*out_value = e->value;
				}

				return false;
			}

			elem = e;
			break;
		}
	}

	if(transform_value != NULL) {
		value = transform_value(value);
	}

	if(out_value != NULL) {
		*out_value = value;
	}

	if(elem == NULL) {
		elem = malloc(sizeof(*elem));
		HT_FUNC_COPY_KEY(&elem->key, key);
		elem->hash = hash; // HT_FUNC_HASH_KEY(elem->key);
		elem->value = value;
		list_push(elems, elem);
		++ht->num_elements;
		return true;
	}

	elem->value = value;
	return false;
}

HT_DECLARE_PRIV_FUNC(void, check_elem_count, (HT_BASETYPE *ht)) {
	#ifdef DEBUG
	size_t num_elements = 0;
	for(size_t i = 0; i < ht->table_size; ++i) {
		for(HT_TYPE(element) *e = ht->table[i]; e; e = e->next) {
			++num_elements;
		}
	}
	assert(num_elements == ht->num_elements);
	#endif // DEBUG
}

HT_DECLARE_PRIV_FUNC(void, resize, (HT_BASETYPE *ht, size_t new_size)) {
	assert(new_size != ht->table_size);
	HT_TYPE(element) **new_table = calloc(new_size, sizeof(*new_table));
	size_t new_hash_mask = new_size - 1;
	size_t num_elements = ht->num_elements;

	HT_PRIV_FUNC(check_elem_count)(ht);

	for(size_t i = 0; i < ht->table_size; ++i) {
		for(HT_TYPE(element) *e = ht->table[i]; e; e = e->next) {
			HT_PRIV_FUNC(set)(ht, new_table, new_hash_mask, e->hash, e->key, e->value, NULL, true, NULL);
		}
	}

	HT_PRIV_FUNC(unset_all)(ht);
	ht->num_elements = num_elements;

	free(ht->table);
	ht->table = new_table;

	log_debug(
		"Resized hashtable at %p: %"PRIuMAX" -> %"PRIuMAX"",
		(void*)ht, (uintmax_t)ht->table_size, (uintmax_t)new_size
	);

	ht->table_size = new_size;
	ht->hash_mask = new_hash_mask;

	HT_PRIV_FUNC(check_elem_count)(ht);
}

HT_DECLARE_FUNC(bool, set, (HT_BASETYPE *ht, HT_TYPE(const_key) key, HT_TYPE(value) value)) {
	hash_t hash = HT_FUNC_HASH_KEY(key);

	HT_PRIV_FUNC(begin_write)(ht);
	bool result = HT_PRIV_FUNC(set)(ht, ht->table, ht->hash_mask, hash, key, value, NULL, true, NULL);

	if(ht->num_elements == ht->table_size) {
		HT_PRIV_FUNC(resize)(ht, ht->table_size * 2);
		assert(ht->num_elements == ht->table_size / 2);
	}
	HT_PRIV_FUNC(end_write)(ht);

	return result;
}

HT_DECLARE_FUNC(bool, try_set, (HT_BASETYPE *ht, HT_TYPE(const_key) key, HT_TYPE(value) value, HT_TYPE(value) (*value_transform)(HT_TYPE(value)), HT_TYPE(value) *out_value)) {
	hash_t hash = HT_FUNC_HASH_KEY(key);

	HT_PRIV_FUNC(begin_write)(ht);
	bool result = HT_PRIV_FUNC(set)(ht, ht->table, ht->hash_mask, hash, key, value, value_transform, false, out_value);

	if(ht->num_elements == ht->table_size) {
		HT_PRIV_FUNC(resize)(ht, ht->table_size * 2);
		assert(ht->num_elements == ht->table_size / 2);
	}

	HT_PRIV_FUNC(end_write)(ht);
	return result;
}

HT_DECLARE_FUNC(void*, foreach, (HT_BASETYPE *ht, HT_TYPE(foreach_callback) callback, void *arg)) {
	void *ret = NULL;

	HT_PRIV_FUNC(begin_read)(ht);

	for(size_t i = 0; i < ht->table_size; ++i) {
		for(HT_TYPE(element) *e = ht->table[i]; e; e = e->next) {
			ret = callback(e->key, e->value, arg);
			if(ret != NULL) {
				goto end;
			}
		}
	}

end:
	HT_PRIV_FUNC(end_read)(ht);
	return ret;
}

HT_DECLARE_PRIV_FUNC(void, iter_advance, (HT_BASETYPE *ht, HT_TYPE(iter) *iter)) {
	while(!iter->private.elem) {
		if(++iter->private.bucketnum == ht->table_size) {
			iter->has_data = false;
			return;
		}

		iter->private.elem = ht->table[iter->private.bucketnum];
	}

	iter->key = iter->private.elem->key;
	iter->value = iter->private.elem->value;
}

HT_DECLARE_FUNC(void, iter_begin, (HT_BASETYPE *ht, HT_TYPE(iter) *iter)) {
	HT_PRIV_FUNC(begin_read)(ht);
	memset(iter, 0, sizeof(*iter));
	iter->hashtable = ht;
	iter->private.elem = *ht->table;
	iter->has_data = true;
	HT_PRIV_FUNC(iter_advance)(ht, iter);
}

HT_DECLARE_FUNC(void, iter_next, (HT_TYPE(iter) *iter)) {
	if(!iter->has_data) {
		return;
	}

	iter->private.elem = iter->private.elem->next;
	HT_PRIV_FUNC(iter_advance)(iter->hashtable, iter);
	return;
}

HT_DECLARE_FUNC(void, iter_end, (HT_TYPE(iter) *iter)) {
	HT_PRIV_FUNC(end_read)(iter->hashtable);
}

#endif // HT_IMPL

/***********\
 * Cleanup *
\***********/

// Generated with:
// $ sed -e 's/.*#define\s*//' -e 's/[^A-Z0-9_].*//' -e '/^$/d' -e 's/^/#undef /' hashtable.inc.h | sort -u
// I'm sure this can be done much more efficiently, but I don't care to figure it out.

// It is intentional that it also catches the examples in comments.
// The user-supplied macros have to be #undef'd too.

#undef HT_BASETYPE
#undef HT_DECL
#undef HT_DECLARE_FUNC
#undef HT_DECLARE_PRIV_FUNC
#undef HT_FUNC
#undef HT_FUNC_COPY_KEY
#undef HT_FUNC_FREE_KEY
#undef HT_FUNC_HASH_KEY
#undef HT_FUNC_KEYS_EQUAL
#undef HT_IMPL
#undef HT_INLINE
#undef HT_KEY_CONST
#undef HT_KEY_TYPE
#undef HT_MIN_SIZE
#undef HT_NAME
#undef HT_PRIV_FUNC
#undef HT_PRIV_NAME
#undef HT_SUFFIX
#undef HT_THREAD_SAFE
#undef HT_TYPE
#undef HT_VALUE_CONST
#undef HT_VALUE_TYPE
#undef _HT_NAME_INNER1
#undef _HT_NAME_INNER2
#undef _HT_PRIV_NAME_INNER1
#undef _HT_PRIV_NAME_INNER2
