
// Sets up the predefined hashtable types.
// Add more as necessary.

#ifdef HT_IMPL
	#define _HT_IMPL
#endif

#ifdef HT_DECL
	#define _HT_DECL
#endif

/*
 * str2ptr
 *
 * Maps strings to void pointers.
 */
#define HT_SUFFIX                      str2ptr
#define HT_KEY_TYPE                    char*
#define HT_VALUE_TYPE                  void*
#define HT_FUNC_FREE_KEY(key)          free(key)
#define HT_FUNC_KEYS_EQUAL(key1, key2) (!strcmp(key1, key2))
#define HT_FUNC_HASH_KEY(key)          htutil_hashfunc_string(key)
#define HT_FUNC_COPY_KEY(dst, src)     (*(dst) = strdup(src))
#define HT_KEY_FMT                     "s"
#define HT_KEY_PRINTABLE(key)          (key)
#define HT_VALUE_FMT                   "p"
#define HT_VALUE_PRINTABLE(val)        (val)
#define HT_KEY_CONST
#define HT_VALUE_CONST
#include "hashtable_incproxy.inc.h"

/*
 * str2ptr_ts
 *
 * Maps strings to void pointers (thread-safe).
 */
#define HT_SUFFIX                      str2ptr_ts
#define HT_KEY_TYPE                    char*
#define HT_VALUE_TYPE                  void*
#define HT_FUNC_FREE_KEY(key)          free(key)
#define HT_FUNC_KEYS_EQUAL(key1, key2) (!strcmp(key1, key2))
#define HT_FUNC_HASH_KEY(key)          htutil_hashfunc_string(key)
#define HT_FUNC_COPY_KEY(dst, src)     (*(dst) = strdup(src))
#define HT_KEY_FMT                     "s"
#define HT_KEY_PRINTABLE(key)          (key)
#define HT_VALUE_FMT                   "p"
#define HT_VALUE_PRINTABLE(val)        (val)
#define HT_KEY_CONST
#define HT_VALUE_CONST
#define HT_THREAD_SAFE
#include "hashtable_incproxy.inc.h"

/*
 * str2int
 *
 * Maps strings to 64-bit integers.
 */
#define HT_SUFFIX                      str2int
#define HT_KEY_TYPE                    char*
#define HT_VALUE_TYPE                  int64_t
#define HT_FUNC_FREE_KEY(key)          free(key)
#define HT_FUNC_KEYS_EQUAL(key1, key2) (!strcmp(key1, key2))
#define HT_FUNC_HASH_KEY(key)          htutil_hashfunc_string(key)
#define HT_FUNC_COPY_KEY(dst, src)     (*(dst) = strdup(src))
#define HT_KEY_FMT                     "s"
#define HT_KEY_PRINTABLE(key)          (key)
#define HT_VALUE_FMT                   PRIi64
#define HT_VALUE_PRINTABLE(val)        (val)
#define HT_KEY_CONST
#include "hashtable_incproxy.inc.h"

/*
 * str2int_ts
 *
 * Maps strings to 64-bit integers.
 */
#define HT_SUFFIX                      str2int_ts
#define HT_KEY_TYPE                    char*
#define HT_VALUE_TYPE                  int64_t
#define HT_FUNC_FREE_KEY(key)          free(key)
#define HT_FUNC_KEYS_EQUAL(key1, key2) (!strcmp(key1, key2))
#define HT_FUNC_HASH_KEY(key)          htutil_hashfunc_string(key)
#define HT_FUNC_COPY_KEY(dst, src)     (*(dst) = strdup(src))
#define HT_KEY_FMT                     "s"
#define HT_KEY_PRINTABLE(key)          (key)
#define HT_VALUE_FMT                   PRIi64
#define HT_VALUE_PRINTABLE(val)        (val)
#define HT_KEY_CONST
#define HT_THREAD_SAFE
#include "hashtable_incproxy.inc.h"

/*
 * int2int
 *
 * Maps 64-bit integers to 64-bit integers.
 */
#define HT_SUFFIX                      int2int
#define HT_KEY_TYPE                    int64_t
#define HT_VALUE_TYPE                  int64_t
#define HT_FUNC_HASH_KEY(key)          htutil_hashfunc_uint64((uint64_t)(key))
#define HT_KEY_FMT                     PRIi64
#define HT_KEY_PRINTABLE(key)          (key)
#define HT_VALUE_FMT                   PRIi64
#define HT_VALUE_PRINTABLE(val)        (val)
#include "hashtable_incproxy.inc.h"

/*
 * int2int_ts
 *
 * Maps 64-bit integers to 64-bit integers (thread-safe).
 */
#define HT_SUFFIX                      int2int_ts
#define HT_KEY_TYPE                    int64_t
#define HT_VALUE_TYPE                  int64_t
#define HT_FUNC_HASH_KEY(key)          htutil_hashfunc_uint64((uint64_t)(key))
#define HT_KEY_FMT                     PRIi64
#define HT_KEY_PRINTABLE(key)          (key)
#define HT_VALUE_FMT                   PRIi64
#define HT_VALUE_PRINTABLE(val)        (val)
#define HT_THREAD_SAFE
#include "hashtable_incproxy.inc.h"

/*
 * int2ptr
 *
 * Maps 64-bit integers to pointers.
 */
#define HT_SUFFIX                      int2ptr
#define HT_KEY_TYPE                    int64_t
#define HT_VALUE_TYPE                  void*
#define HT_FUNC_HASH_KEY(key)          htutil_hashfunc_uint64((uint64_t)(key))
#define HT_KEY_FMT                     PRIi64
#define HT_KEY_PRINTABLE(key)          (key)
#define HT_VALUE_FMT                   "p"
#define HT_VALUE_PRINTABLE(val)        (val)
#include "hashtable_incproxy.inc.h"

/*
 * int2ptr_ts
 *
 * Maps 64-bit integers to pointers (thread-safe).
 */
#define HT_SUFFIX                      int2ptr_ts
#define HT_KEY_TYPE                    int64_t
#define HT_VALUE_TYPE                  void*
#define HT_FUNC_HASH_KEY(key)          htutil_hashfunc_uint64((uint64_t)(key))
#define HT_KEY_FMT                     PRIi64
#define HT_KEY_PRINTABLE(key)          (key)
#define HT_VALUE_FMT                   "p"
#define HT_VALUE_PRINTABLE(val)        (val)
#define HT_THREAD_SAFE
#include "hashtable_incproxy.inc.h"

/*
 * ptr2int
 *
 * Maps pointers to 64-bit integers.
 */
#define HT_SUFFIX                      ptr2int
#define HT_KEY_TYPE                    void*
#define HT_VALUE_TYPE                  int64_t
#define HT_FUNC_HASH_KEY(key)          htutil_hashfunc_uint64((uintptr_t)(key))
#define HT_FUNC_COPY_KEY(dst, src)     (*(dst) = (void*)(src))
#define HT_KEY_FMT                     "p"
#define HT_KEY_PRINTABLE(key)          (key)
#define HT_VALUE_FMT                   PRIi64
#define HT_VALUE_PRINTABLE(val)        (val)
#define HT_KEY_CONST
#include "hashtable_incproxy.inc.h"

/*
 * ptr2int_ts
 *
 * Maps pointers to 64-bit integers (thread-safe).
 */
#define HT_SUFFIX                      ptr2int_ts
#define HT_KEY_TYPE                    void*
#define HT_VALUE_TYPE                  int64_t
#define HT_FUNC_HASH_KEY(key)          htutil_hashfunc_uint64((uintptr_t)(key))
#define HT_FUNC_COPY_KEY(dst, src)     (*(dst) = (void*)(src))
#define HT_KEY_FMT                     "p"
#define HT_KEY_PRINTABLE(key)          (key)
#define HT_VALUE_FMT                   PRIi64
#define HT_VALUE_PRINTABLE(val)        (val)
#define HT_KEY_CONST
#define HT_THREAD_SAFE
#include "hashtable_incproxy.inc.h"

/*
 * ptr2ptr
 *
 * Maps pointers to pointers.
 */
#define HT_SUFFIX                      ptr2ptr
#define HT_KEY_TYPE                    void*
#define HT_VALUE_TYPE                  void*
#define HT_FUNC_HASH_KEY(key)          htutil_hashfunc_uint64((uintptr_t)(key))
#define HT_FUNC_COPY_KEY(dst, src)     (*(dst) = (void*)(src))
#define HT_KEY_FMT                     "p"
#define HT_KEY_PRINTABLE(key)          (key)
#define HT_VALUE_FMT                   "p"
#define HT_VALUE_PRINTABLE(val)        (val)
#define HT_KEY_CONST
#include "hashtable_incproxy.inc.h"

/*
 * ptr2ptr_ts
 *
 * Maps pointers to pointers (thread-safe).
 */
#define HT_SUFFIX                      ptr2ptr_ts
#define HT_KEY_TYPE                    void*
#define HT_VALUE_TYPE                  void*
#define HT_FUNC_HASH_KEY(key)          htutil_hashfunc_uint64((uintptr_t)(key))
#define HT_FUNC_COPY_KEY(dst, src)     (*(dst) = (void*)(src))
#define HT_KEY_FMT                     "p"
#define HT_KEY_PRINTABLE(key)          (key)
#define HT_VALUE_FMT                   "p"
#define HT_VALUE_PRINTABLE(val)        (val)
#define HT_KEY_CONST
#define HT_THREAD_SAFE
#include "hashtable_incproxy.inc.h"

/*
 * C11 generic selection witchcraft.
 */

#define _HT_GENERIC_MAP(suffix, name) ht_##suffix##_t : ht_##suffix##_##name

// Add all the non-thread-safe types here.
#define _HT_GENERICLIST_NONTS(name) \
	_HT_GENERIC_MAP(str2ptr, name), \
	_HT_GENERIC_MAP(str2int, name), \
	_HT_GENERIC_MAP(int2int, name), \
	_HT_GENERIC_MAP(int2ptr, name), \
	_HT_GENERIC_MAP(ptr2int, name), \
	_HT_GENERIC_MAP(ptr2ptr, name)

// Add all the thread-safe types here.
#define _HT_GENERICLIST_TS(name) \
	_HT_GENERIC_MAP(str2ptr_ts, name), \
	_HT_GENERIC_MAP(str2int_ts, name), \
	_HT_GENERIC_MAP(int2int_ts, name), \
	_HT_GENERIC_MAP(int2ptr_ts, name), \
	_HT_GENERIC_MAP(ptr2int_ts, name), \
	_HT_GENERIC_MAP(ptr2ptr_ts, name)

#define _HT_GENERIC(ht_expr, name) (_Generic((ht_expr), \
	 _HT_GENERICLIST_NONTS(name), \
	 _HT_GENERICLIST_TS(name) \
))

#define _HT_GENERIC_TS(ht_expr, name) (_Generic((ht_expr), \
	 _HT_GENERICLIST_TS(name) \
))

/*
 * Type-generic wrappers around the core API.
 *
 * For the core API documentation, see hashtable.inc.h
 *
 * To find documentation about a macro called "ht_foo", search hashtable.inc.h
 * for "ht_XXX_foo" (the XXX is literal).
 */

#define ht_create(ht) \
	_HT_GENERIC(*(ht), create) (ht)

#define ht_destroy(ht) \
	_HT_GENERIC(*(ht), destroy) (ht)

#define ht_lock(ht) \
	_HT_GENERIC_TS(*(ht), lock) (ht)

#define ht_unlock(ht) \
	_HT_GENERIC(*(ht), unlock) (ht)

#define ht_get(ht, key, default) \
	_HT_GENERIC(*(ht), get) (ht, key, default)

#define ht_get_prehashed(ht, key, hash, default) \
	_HT_GENERIC(*(ht), get_prehashed) (ht, key, hash, default)

#define ht_get_unsafe(ht, key, default) \
	_HT_GENERIC_TS(*(ht), get_unsafe) (ht, key, default)

#define ht_get_unsafe_prehashed(ht, key, hash, default) \
	_HT_GENERIC_TS(*(ht), get_unsafe_prehashed) (ht, key, hash, default)

#define ht_lookup(ht, key, out_value) \
	_HT_GENERIC(*(ht), lookup) (ht, key, out_value)

#define ht_lookup_prehashed(ht, key, hash, out_value) \
	_HT_GENERIC(*(ht), lookup_prehashed) (ht, key, hash, out_value)

#define ht_lookup_unsafe(ht, key, out_value) \
	_HT_GENERIC_TS(*(ht), lookup_unsafe) (ht, key, out_value)

#define ht_lookup_unsafe_prehashed(ht, key, hash, out_value) \
	_HT_GENERIC_TS(*(ht), lookup_unsafe_prehashed) (ht, key, hash, out_value)

#define ht_set(ht, key, value) \
	_HT_GENERIC(*(ht), set) (ht, key, value)

#define ht_try_set(ht, key, value, value_transform, out_value) \
	_HT_GENERIC(*(ht), try_set) (ht, key, value, value_transform, out_value)

#define ht_try_set_prehashed(ht, key, hash, value, value_transform, out_value) \
	_HT_GENERIC(*(ht), try_set_prehashed) (ht, key, hash, value, value_transform, out_value)

#define ht_unset(ht, key) \
	_HT_GENERIC(*(ht), unset) (ht, key)

#define ht_unset_list(ht, keylist) \
	_HT_GENERIC(*(ht), unset_list) (ht, keylist)

#define ht_unset_all(ht) \
	_HT_GENERIC(*(ht), unset_all) (ht)

#define ht_foreach(ht, callback, arg) \
	_HT_GENERIC(*(ht), foreach) (ht, callback, arg)

#define ht_iter_begin(ht, iter) \
	_HT_GENERIC(*(ht), iter_begin) (ht, iter)

#define ht_iter_next(iter) \
	_HT_GENERIC(*((iter)->hashtable), iter_next) (iter)

#define ht_iter_end(iter) \
	_HT_GENERIC(*((iter)->hashtable), iter_end) (iter)

/*
 * Cleanup
 */
#undef _HT_IMPL
#undef _HT_DECL
