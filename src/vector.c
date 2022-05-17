/*
 * vector -- Implementation of vectors with dynamic arrays
 */

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
//#include <pthread.h>

#include "vector.h"

#define ASSERT_PRECONDITION(cond, action) \
	if (!(cond))                      \
	action

enum vec_consts {
	VEC_NEXTSIZE = 0
};

struct vector {
	/* number of objects in the vector */
	size_t size;

	/* number of objects can be hold without reallocating */
	size_t capacity;

	/* size of an object */
	size_t objsz;

	/* if false, this vector is immutable */
	bool mutable;

	/* beginning of the dynamic array holding objects */
	char *data;

	/* mutex for thread safety */
	// pthread_mutex_t lock;
};

struct vector_iter {
	/* The vector to iterate */
	struct vector *v;

	/* beginning index of the iteration */
	size_t begin;

	/* ending index of the iteration */
	size_t end;

	/* current index */
	size_t current;
};

static size_t __vector_default_growby(size_t sz);

static allocator_fn   __vec_alloc   = &malloc;
static deallocator_fn __vec_dealloc = &free;
static growby_fn      __vec_growby  = &__vector_default_growby;

static inline bool __vector_idx_is_valid(struct vector *v, size_t idx)
{
	return v && idx >= 0 && idx < v->capacity;
}

static inline char *__vector_idx_to_ptr(struct vector *v, size_t idx)
{
	// v is assumed to be not NULL and v->data is valid
	return v->data + (idx * v->objsz);
}

static inline bool __vector_is_valid(struct vector *v)
{
	return v && v->data;
}

static inline bool __vector_needs_realloc_for(struct vector *v, size_t idx)
{
	return idx > v->capacity;
}

static size_t __vector_default_growby(size_t sz)
{
	// first time grow for empty vectors
	if (sz == 0)
		return 1;

	// We need to keep the size as a power of 2
	if ((sz & (sz - 1)) == 0)
		return sz << 1;

	size_t y = 1;
	while (y > sz)
		y <<= 1;
	return y;
}

static int __vector_realloc(struct vector *v, size_t atleast)
{
	size_t cursz = atleast ? atleast : v->capacity;
	size_t newsz = __vec_growby(cursz) * v->objsz;

	if (newsz <= v->capacity)
		return VEC_EMAXED;

	char *newp = __vec_alloc(newsz);
	if (!newp)
		return VEC_ENOMEM;

	memcpy(newp, v->data, v->size);
	free(v->data);
	v->data	    = newp;
	v->capacity = newsz;
	return VEC_SUCCESS;
}

allocator_fn vector_allocator(allocator_fn alloc)
{
	void *(*old)(size_t) = __vec_alloc;
	if (alloc)
		__vec_alloc = alloc;
	return old;
}

deallocator_fn vector_deallocator(deallocator_fn dealloc)
{
	void (*old)(void *) = __vec_dealloc;
	if (dealloc)
		__vec_dealloc = dealloc;
	return old;
}

growby_fn vector_growby(growby_fn growby)
{
	size_t (*old)(size_t) = __vec_growby;
	if (growby)
		__vec_growby = growby;
	return old;
}

struct vector *vector_new(size_t nobj, size_t objsz)
{
	struct vector *v = __vec_alloc(sizeof *v);
	if (!v)
		return NULL;

	char *arr = NULL;

	if (nobj > 0) {
		arr = __vec_alloc(nobj * objsz);
		if (!arr) {
			__vec_dealloc(v);
			return NULL;
		}
		memset(arr, 0, nobj * objsz);
	}

	v->data	    = arr;
	v->size	    = 0;
	v->objsz    = objsz;
	v->capacity = nobj;
	v->mutable  = true;
	// v->lock	    = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;

	return v;
}

void vector_free(struct vector **vp, void (*elem_dtor)(void *))
{
	ASSERT_PRECONDITION((vp && (*vp)), return );

	struct vector *v = *vp;

	if (v->data && elem_dtor) {
		for (size_t i = 0; i < v->size; i++)
			elem_dtor(v->data + (i * v->objsz));
	}

	if (v->data)
		__vec_dealloc(v->data);

	__vec_dealloc(v);

	*vp = NULL;
}

size_t vector_size(struct vector *v)
{
	ASSERT_PRECONDITION(v != NULL, return 0);
	return v->size;
}

size_t vector_capacity(struct vector *v)
{
	ASSERT_PRECONDITION(v != NULL, return 0);
	return v->capacity;
}

bool vector_is_empty(struct vector *v)
{
	ASSERT_PRECONDITION(v != NULL, return true);
	return v->size == 0;
}

bool vector_is_mutable(struct vector *v)
{
	// invalid vectors are immutable
	ASSERT_PRECONDITION(__vector_is_valid(v), return true);
	return v->mutable;
}

int vector_reserve(struct vector *v, size_t size)
{
	ASSERT_PRECONDITION(v != NULL && size > 0, return VEC_EINVAL);
	ASSERT_PRECONDITION(v->mutable, return VEC_EIMMUT);

	return __vector_realloc(v, size);
}

int vector_make_immutable(struct vector *v)
{
	return vector_fit(v, true);
}

int vector_fit(struct vector *v, bool immutable)
{
	ASSERT_PRECONDITION(__vector_is_valid(v), return VEC_EINVAL);

	if (v->size < v->capacity) {
		char *fitp = __vec_alloc(v->size);
		if (!fitp)
			return VEC_ENOMEM;

		memcpy(fitp, v->data, v->size);
		v->capacity = v->size;
		free(v->data);
		v->data = fitp;
	}
	v->mutable = !immutable;

	return VEC_SUCCESS;
}

int vector_get(struct vector *v, size_t idx, void *p)
{
	ASSERT_PRECONDITION(__vector_is_valid(v) && p != NULL, return VEC_EINVAL);
	ASSERT_PRECONDITION(__vector_idx_is_valid(v, idx), return VEC_ERANGE);

	char *el = __vector_idx_to_ptr(v, idx);
	memcpy(p, el, v->objsz);
	return VEC_SUCCESS;
}

int vector_insert(struct vector *v, size_t idx, void *p)
{
	ASSERT_PRECONDITION(__vector_is_valid(v) && p != NULL, return VEC_EINVAL);

	/* try to resize the vector if idx exceed the capacity */
	int res;
	if (__vector_needs_realloc_for(v, idx)
	    && (res = __vector_realloc(v, VEC_NEXTSIZE)) != VEC_SUCCESS)
		return res;

	char *el = __vector_idx_to_ptr(v, idx);
	memcpy(el, p, v->objsz);

	/* when idx exceed current size, advance size to this index.
	 * this will absorb uninitialized objects in the middle
	 */
	if (v->size <= idx)
		v->size = idx + 1;

	return VEC_SUCCESS;
}

int vector_erase(struct vector *v, size_t idx)
{
	ASSERT_PRECONDITION(__vector_is_valid(v), return VEC_EINVAL);
	ASSERT_PRECONDITION(v->mutable, return VEC_EIMMUT);
	ASSERT_PRECONDITION(__vector_idx_is_valid(v, idx), return VEC_ERANGE);

	char *el = __vector_idx_to_ptr(v, idx);
	memset(el, 0, v->objsz);

	if (idx == v->size - 1)
		v->size--;

	return VEC_SUCCESS;
}

int vector_push(struct vector *v, void *p)
{
	return vector_insert(v, v->size, p);
}

int vector_pop(struct vector *v, void *p)
{
	ASSERT_PRECONDITION(v->mutable, return VEC_EIMMUT);
	int res = vector_get(v, v->size - 1, p);
	if (res != VEC_SUCCESS)
		return res;

	return vector_erase(v, v->size - 1);
}

struct vector_iter *vector_get_iterator(struct vector *v, size_t begin, size_t end)
{
	ASSERT_PRECONDITION(__vector_is_valid(v), return NULL);
	ASSERT_PRECONDITION(__vector_idx_is_valid(v, begin) && __vector_idx_is_valid(v, end), return NULL);
	ASSERT_PRECONDITION(begin <= end, return NULL);

	struct vector_iter *it = __vec_alloc(sizeof *it);
	if (!it)
		return NULL;

	it->v	    = v;
	it->begin   = begin;
	it->end	    = end;
	it->current = begin;

	return it;
}

bool vector_has_next(struct vector_iter *it)
{
	ASSERT_PRECONDITION(it != NULL && it->v != NULL, return false);

	return it->current >= it->begin && it->current < it->end;
}

int vector_get_next(struct vector_iter *it, void *p)
{
	ASSERT_PRECONDITION(it != NULL && it->v != NULL && p != NULL, return VEC_EINVAL);
	ASSERT_PRECONDITION(vector_has_next(it), return VEC_EITEHX);

	it->current++;
	return vector_get(it->v, it->current - 1, p);
}

void vector_reset_iterator(struct vector_iter *it)
{
	ASSERT_PRECONDITION(it != NULL, return );
	it->current = it->begin;
}

void vector_free_iterator(struct vector_iter *it)
{
	ASSERT_PRECONDITION(it != NULL, return );
	__vec_dealloc(it);
}
