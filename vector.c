/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * vector -- Implementation of vectors with dynamic arrays
 */

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

enum {
	VEC_ERANGE = -1,
	VEC_EINVAL = -2
};

void *(*alloc)(size_t)	= &malloc;
void (*dealloc)(void *) = &free;

struct vector {
	size_t count;
	size_t capacity;
	size_t elemsz;
	size_t end;

	char *data;
};

static inline int is_valid_idx(struct vector *v, size_t idx)
{
	return v && idx >= 0 && idx < v->capacity;
}

void *(*vector_set_allocator(void *(*new)(size_t)))(size_t)
{
	if (!new)
		return alloc;

	void *(*old)(size_t) = alloc;
	alloc		     = new;
	return old;
}

void (*vector_set_deallocator(void (*new)(void *)))(void *)
{
	if (!new)
		return dealloc;

	void (*old)(void *) = dealloc;
	dealloc		    = new;
	return old;
}

struct vector *vector_new(size_t init, size_t elemsz)
{
	struct vector *v = alloc(sizeof *v);
	if (!v)
		return NULL;

	char *arr = alloc(init * elemsz);
	if (!arr) {
		dealloc(v);
		return NULL;
	}

	memset(arr, 0, init * elemsz);
	v->data	    = arr;
	v->count    = 0;
	v->elemsz   = elemsz;
	v->capacity = init;

	return v;
}

void vector_free(struct vector **vp, void (*elem_dtor)(void *))
{
	if (!vp || !(*vp))
		return;

	struct vector *v = *vp;

	if (v->data && elem_dtor) {
		for (size_t i = 0; i < v->count; i++)
			elem_dtor(v->data + (i * v->elemsz));
	}

	if (v->data)
		dealloc(v->data);

	dealloc(v);

	*vp = NULL;
}

int vector_get(struct vector *v, size_t idx, void *p)
{
	if (!is_valid_idx(v, idx))
		return VEC_ERANGE;

	if (!v->data)
		return VEC_EINVAL;

	char *el = v->data + (idx * v->elemsz);
	memcpy(p, el, v->elemsz);
	return 0;
}

int vector_set(struct vector *v, size_t idx, void *p)
{
	return 0;
}
