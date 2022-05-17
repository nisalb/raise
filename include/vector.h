/**
 * @file
 * The Vector Interface
 *
 * This header defines the interface for vectors for C.
 */

#ifndef ASMS_VECTOR_H
#define ASMS_VECTOR_H

#include <stdbool.h>
#include <stddef.h>

/**
 * List of possible error codes for several vector functions
 */
enum vec_error {
	VEC_SUCCESS = 0,  /**< No errors. Operation successfully. */
	VEC_ERANGE  = -1, /**< Index out of range. */
	VEC_EINVAL  = -2, /**< Invalid vector. */
	VEC_EMAXED  = -3, /**< Vector has reached maximum growth allowed. */
	VEC_EIMMUT  = -4, /**< Vector is immutable. */
	VEC_ENOMEM  = -5, /**< Memory allocation failed. */
	VEC_EITEHX  = -6  /**< Exhausted iterator */
};

/**
 * Type of an allocator function. Same as `malloc`
 */
typedef void *(*allocator_fn)(size_t);

/**
 * Type of a deallocator function. Same as `free`
 */
typedef void (*deallocator_fn)(void *);

/**
 * Type of a growby function. A growby function accepts a
 * `size_t` parameter and returns the next available size for
 * the vector to grow.
 */
typedef size_t (*growby_fn)(size_t);

/**
 * Set or get the allocator used for internal allocations
 *
 * @param alloc A @c malloc like function. If @c NULL nothing is changed.
 * @returns The existing allocator.
 */
// void *(*vector_allocator(void *(*alloc)(size_t)))(size_t);
allocator_fn vector_allocator(allocator_fn alloc);

/**
 * Set or get the deallocator used for internal deallocations
 *
 * @param dealloc A @c free like function. If @c NULL nothing is changed.
 * @returns The existing allocator.
 */
// void (*vector_deallocator(void (*dealloc)(void *)))(void *);
deallocator_fn vector_deallocator(deallocator_fn dealloc);

/**
 * Set or get the growth factor (growby) for vectors.
 *
 * A growth factor is a function accepting a size value and returns
 * the next possible size for that value. The default growth factor is
 * to return the least power of 2 bigger than the given size.
 *   eg. @c default_growby(10) -> 16
 *
 * @param growby A pointer to a function accepting a size and returning a size.
 * @returns The existing growth factor.
 */
// size_t (*vector_growby(size_t (*growby)(size_t)))(size_t);
growby_fn vector_growby(growby_fn growby);

/**
 * Initialize a new vector
 *
 * This function allocates for the vector structure and an array of @c init objects
 * of @c objsz size. It returns @c NULL only when memory allocation fails.
 *
 * @param nobj Number of objects to allocate at the initialization
 *        (could be 0 for empty vectors).
 * @param objsz Number of bytes occupied by each object. Must be >= 0.
 * @returns A pointer to the vector object, which must be freed with `@c vector_free(). @c NULL when memory allocation failed.
 */
struct vector *vector_new(size_t nobj, size_t objsz);

/**
 * Free the resources allocated by the vector
 *
 * This function requires a pointer to the vector object pointer, since it will be set
 * to @c NULL after freeing up resources. Additionally the @c elem_dtor function
 * will be called on each object in the vector before freeing up the data array.
 *
 * @param vp A pointer to the vector pointer. @c *vp is @c NULL after calling this.
 * @param elem_dtor A pointer to object deconstructor function.
 */
void vector_free(struct vector **vp, void (*elem_dtor)(void *));

/**
 * Get the size of the vector.
 *
 * @param v The vector pointer.
 * @returns The number of objects in the vector, 0 if @c v is @c NULL.
 */
size_t vector_size(struct vector *v);

/**
 * Get the current capacity of the vector.
 *
 * @param v The vector pointer
 * @returns The number of objects fit into the current array. 0 if @c v is @c NULL.
 */
size_t vector_capacity(struct vector *v);

/**
 * Returns @c true if the vector is empty.
 *
 * @param v The vector pointer
 * @returns @c true if the vector is empty. Also @c true if @c v is @c NULL.
 */
bool vector_is_empty(struct vector *v);

/**
 * Returns @c true if the vector is mutable.
 *
 * Sometimes vectors needs to marked as immutable to prevent unwanted mutations.
 * A Vector is created, filled and marked immutable to make sure it is readonly.
 *
 * @param v The vector pointer.
 * @returns @c true if the vector is mutable. Also @c true if @c v is @c NULL.
 */
bool vector_is_mutable(struct vector *v);

/**
 * Reserve at least @c size objects for the vector.
 *
 * This will try to allocate at least @c size objects for the vector.
 * Actual allocation size may or may not be equal to @c size, but it
 * will be at least @c size objects.
 *
 * @param v The vector pointer.
 * @param size At least this number of object will be reserved.
 * @returns @c VEC_SUCCESS on success, otherwise a an error code as in <tt>enum vec_error</tt>.
 * @see enum vec_error
 */
int vector_reserve(struct vector *v, size_t size);

/**
 * Make the vector immutable.
 *
 * After calling this function the vector will not accept anymore additions
 * or removals from the array. The underlying array is also shrink to the actual size
 * of the vector. This is same as <tt>vector_fit(v, true)</tt>.
 *
 * @param v The vector pointer.
 * @returns @c VEC_SUCCESS on success, otherwise an error code as in <tt>enum vec_error</tt>.
 * @see enum vec_error
 */
int vector_make_immutable(struct vector *v);

/**
 * Make the vector capacity equal to its size.
 *
 * This will result in re-allocations but the objects are left untouched.
 *
 * @param v The vector pointer.
 * @param immutable If @c true vector is also marked as immutable.
 * @returns @c VEC_SUCCESS on success, otherwise an error code as in <tt>enum vec_error</tt>.
 * @see enum vec_error
 */
int vector_fit(struct vector *v, bool immutable);

/**
 * Get the object at the given index.
 *
 * The retrieved object is stored at the memory pointed by @c p.
 * It is callers responsibility to make sure @c p to be valid
 * to store the object.
 *
 * @param v The vector pointer.
 * @param idx Index of the object.
 * @param p The retrieved object will be stored here. Must not be @c NULL.
 * @return @c VEC_SUCCESS on success, otherwise an error code as in <tt>enum vec_error</tt>.
 * @see enum vec_error
 */
int vector_get(struct vector *v, size_t idx, void *p);

/**
 * Insert an object at the given index.
 *
 * @param v The vector pointer.
 * @param idx Index to store the object at.
 * @param p A pointer to the object.
 * @returns @c VEC_SUCCESS on success, otherwise an error code as in <tt>enum vec_error</tt>.
 */
int vector_insert(struct vector *v, size_t idx, void *p);

/**
 * Clear the object stored at the given index.
 *
 * This will effectively remove an object from the vector.
 * Size will be left unchanged, unless it is the last object
 * in the vector.
 *
 * @param v The vector pointer.
 * @param idx Index of the object to remove.
 * @returns @c VEC_SUCCESS on success, otherwise an error code as in <tt>enum vec_error</tt>.
 */
int vector_erase(struct vector *v, size_t idx);

/**
 * Append an object to the end of the vector.
 *
 * This will make the vector to reallocate if necessary.
 *
 * @param v The vector pointer.
 * @param p A pointer to the object to append.
 * @returns @c VEC_SUCCESS on success, otherwise an error code as in <tt>enum vec_error</tt>.
 */
int vector_push(struct vector *v, void *p);

/**
 * Remove the last object from the vector.
 *
 * Last object of the list will be stored at `p` and removed from the vector.
 *
 * @param v The vector pointer.
 * @param p A pointer to store the object at.
 * @returns @c VEC_SUCCESS on success, otherwise an error code as in <tt>enum vec_error</tt>.
 */
int vector_pop(struct vector *v, void *p);

/**
 * Create a new iterator.
 *
 * @param v The vector pointer. New iterator will be created for this vector.
 * @param begin Beginning index of the vector (inclusive).
 * @param end Ending index of the vector (exclusive).
 * @return A pointer to the vector iterator, which must be freed with @c vector_free_iterator().
 *         @c NULL if memory allocation fails.
 */
struct vector_iter *vector_get_iterator(struct vector *v, size_t begin, size_t end);

/**
 * Returns @c true if the iterator has more items to process.
 *
 * This implies that next call to @c vector_get_next() will be successful.
 *
 * @param it The iterator pointer.
 * @return @c true if iterator has more items. @c false if no more items to process.
 */
bool vector_has_next(struct vector_iter *it);

/**
 * Retrieve next item in the iterator into @c p
 *
 * @param it The iterator pointer .
 * @param p User allocated pointer to store the item in. Must not be @c NULL.
 * @return @c VEC_SUCCESS on success, otherwise an error code as in <tt>enum vec_error</tt>.
 */
int vector_get_next(struct vector_iter *it, void *p);

/**
 * Reset the iterator.
 *
 * @param it The iterator pointer.
 */
void vector_reset_iterator(struct vector_iter *it);

/**
 * Free the allocated resources for the iterator
 *
 * @param it The iterator pointer.
 */
void vector_free_iterator(struct vector_iter *it);

#endif /* ASMS_VECTOR_H */
