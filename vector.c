/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * vector -- Implementation of vectors with dynamic arrays
 */

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

#define REALLOC_NEXT_SIZE 0

enum vec_error {
	VEC_SUCCESS = 0,
	VEC_ERANGE  = -1,
	VEC_EINVAL  = -2,
	VEC_EMAXED  = -3
};

struct vector {
	/* number of objects in the vector */
	size_t size;

	/* number of objects can be hold without reallocating */
	size_t capacity;

	/* size of an object */
	size_t objsz;

	/* address of the beginning of the last object */
	char *last;

	/* beginning of the dynamic array holding objects */
	char *data;
};

static size_t __vector_default_size_fn(size_t curr);

static void *(*alloc)(size_t)	= &malloc;
static void (*dealloc)(void *)	= &free;
static size_t (*resize)(size_t) = &__vector_default_size_fn;

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

static inline bool __vector_needs_realloc(struct vector *v)
{
	return v->size == v->capacity;
}

static size_t __vector_default_size_fn(size_t sz)
{
	/* We need to keep the size as a power of 2 */
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
	size_t newsz = resize(cursz) * v->objsz;
	if (newsz <= v->capacity)
		return VEC_EMAXED;

	char *newp = alloc(newsz);
	memcpy(newp, v->data, v->size);
	free(v->data);
	v->data	    = newp;
	v->capacity = newsz;
	return VEC_SUCCESS;
}

void *(*vector_set_allocator(void *(*new)(size_t)))(size_t)
{
	void *(*old)(size_t) = alloc;
	if (new)
		alloc = new;
	return old;
}

void (*vector_set_deallocator(void (*new)(void *)))(void *)
{
	void (*old)(void *) = dealloc;
	if (new)
		dealloc = new;
	return old;
}

size_t (*vector_set_sizing_function(size_t (*new)(size_t)))(size_t)
{
	size_t (*old)(size_t) = resize;
	if (new)
		resize = new;
	return old;
}

struct vector *vector_new(size_t init, size_t objsz)
{
	struct vector *v = alloc(sizeof *v);
	if (!v)
		return NULL;

	char *arr = alloc(init * objsz);
	if (!arr) {
		dealloc(v);
		return NULL;
	}

	memset(arr, 0, init * objsz);
	v->data	    = arr;
	v->last	    = v->data;
	v->size	    = 0;
	v->objsz    = objsz;
	v->capacity = init;

	return v;
}

void vector_free(struct vector **vp, void (*elem_dtor)(void *))
{
	if (!vp || !(*vp))
		return;

	struct vector *v = *vp;

	if (v->data && elem_dtor) {
		for (size_t i = 0; i < v->size; i++)
			elem_dtor(v->data + (i * v->objsz));
	}

	if (v->data)
		dealloc(v->data);

	dealloc(v);

	*vp = NULL;
}

size_t vector_size(struct vector *v)
{
	if (!__vector_is_valid(v))
		return VEC_EINVAL;

	return v->size;
}

size_t vector_capacity(struct vector *v)
{
	if (!__vector_is_valid(v))
		return VEC_EINVAL;

	return v->capacity;
}

bool vector_is_empty(struct vector *v)
{
	if (!__vector_is_valid(v))
		return true;

	return v->size == 0;
}

int vector_reserve(struct vector *v, size_t size)
{
	if (!__vector_is_valid(v) || size < 1)
		return VEC_EINVAL;

	return __vector_realloc(v, size);
}

int vector_fit(struct vector *v)
{
	if (!__vector_is_valid(v))
		return VEC_EINVAL;

	char *fitp = alloc(v->size);
	memcpy(fitp, v->data, v->size);
	v->capacity = v->size;
	free(v->data);
	v->data = fitp;

	return VEC_SUCCESS;
}

int vector_get(struct vector *v, size_t idx, void *p)
{
	if (!__vector_is_valid(v))
		return VEC_EINVAL;

	if (!__vector_idx_is_valid(v, idx))
		return VEC_ERANGE;

	char *el = __vector_idx_to_ptr(v, idx);
	memcpy(p, el, v->objsz);
	return VEC_SUCCESS;
}

int vector_insert(struct vector *v, size_t idx, void *p)
{
	if (!__vector_is_valid(v))
		return VEC_EINVAL;

	/* try to resize the vector if idx exceed the capacity */
	if (__vector_needs_realloc_for(v, idx)
	    && __vector_realloc(v, REALLOC_NEXT_SIZE) != VEC_SUCCESS)
		return VEC_EMAXED;

	char *el = __vector_idx_to_ptr(v, idx);
	memcpy(el, p, v->objsz);

	/* when idx exceed current size,
	 * advance size and last to this index.
	 * this will absorb uninitialized objects in the middle
	 */
	if (v->size < idx) {
		v->size = idx;
		v->last = el;
	}

	return VEC_SUCCESS;
}

int vector_push(struct vector *v, void *p)
{
	return vector_insert(v, v->size, p);
}

int vector_pop(struct vector *v, void *p)
{
	return vector_get(v, v->size - 1, p);
}
