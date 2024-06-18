/* Copyright 2024 Bintang Adiputra Pratama <bintangadiputrapratama@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the “Software”), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef MEMPLUS_H__
#define MEMPLUS_H__

/* #define MEMPLUS_IMPLEMENTATION */

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _MEMPLUS_ASSERT
#include <assert.h>
#define _MEMPLUS_ASSERT assert
#endif

#define return_defer(v)                                                                            \
    do {                                                                                           \
        result = (v);                                                                              \
        goto defer;                                                                                \
    } while (0)

/***********
 * ALLOCATOR
 ***********/

/* Default size of a single region in words. You can adjust this to your liking. */
#ifndef MP_REGION_DEFAULT_SIZE
#define MP_REGION_DEFAULT_SIZE (8 * 1024)
#endif

/* Interface to wrap functions to allocate memory.
 * The method of allocation can be costumized by the user. */
typedef struct {
    // The object that manages or holds the memory.
    // In case of allocator that works with global memory, this could be specified as NULL.
    void *context;
    /* These functions is not meant to be used directly.
     * To call them use the macros defined below. */
    // Allocates the memory.
    void *(*alloc)(void *context, size_t size);
    // Takes a pointer to a data and reallocates it with a new size.
    void *(*realloc)(void *context, void *old_ptr, size_t old_size, size_t new_size);
    // Takes a pointer to a data and allocates its duplicate.
    void *(*dup)(void *context, void *data, size_t size);
    // Takes a pointer to a data and deallocates it within the context.
    void (*free)(void *context, void *ptr);
} mp_Allocator;

/* Macros that wrap the functions above */
// allocator: mp_Allocator*
// size: number of bytes
// -> void*
#define mp_alloc(allocator, size) ((allocator)->alloc((allocator)->context, (size)))
// allocator: mp_Allocator*
// old_ptr: pointer
// old_size: number of bytes
// new_size: number of bytes
// -> void*
#define mp_realloc(allocator, old_ptr, old_size, new_size)                                         \
    ((allocator)->realloc((allocator)->context, (old_ptr), (old_size), (new_size)))
// allocator: mp_Allocator*
// data: pointer
// size: number of bytes
// -> void*
#define mp_dup(allocator, data, size) ((allocator)->dup((allocator)->context, (data), (size)))
// allocator: mp_Allocator
// ptr: pointer (nullability depends on the implementation)
#define mp_free(allocator, ptr) ((allocator)->free((allocator)->context, (ptr)))

/* Allocate a new chunk of memory for the given type. */
// allocator: mp_Allocator
// type: typename
// -> `type`*
#define mp_create(allocator, type) ((allocator)->alloc((allocator)->context, (sizeof(type))))

/* Creates a custom allocator given the context and respective function pointers. */
// context: pointer
// alloc_func, realloc_func, dup_func: function pointer
// -> mp_Allocator
#define mp_allocator_new(context, alloc_func, realloc_func, dup_func, free_func)                   \
    ((mp_Allocator){                                                                               \
        (void *) (context),                                                                        \
        (void *(*) (void *, size_t))(alloc_func),                                                  \
        (void *(*) (void *, void *, size_t, size_t))(realloc_func),                                \
        (void *(*) (void *, void *, size_t))(dup_func),                                            \
        (void (*)(void *, void *))(free_func),                                                     \
    })

typedef struct mp_Region mp_Region;

/* Holds certain size of allocated memory. */
struct mp_Region {
    mp_Region *next;        // The next region in the linked list if any
    size_t     size;        // The amount of data (in words) used
    size_t     capacity;    // The amount of data (in words) allocated
    uintptr_t  data[];      // The data (aligned)
};

/* Allocates a new region with `capacity` * sizeof(uintptr_t) bytes of size. */
mp_Region *mp_region_new(size_t capacity);
/* Frees region from memory. */
void mp_region_free(mp_Region *self);

/* GROWING ARENA ALLOCATOR
 * Manages regions in a linked list. */
typedef struct {
    mp_Region *begin, *end;    // Region linked list
    size_t     size;           // The amount of data (in words) used
} mp_Arena;

/* Creates a new, unallocated arena. */
void mp_arena_init(mp_Arena *self);
/* Frees the arena and its regions. */
void mp_arena_destroy(mp_Arena *self);
/* Returns an allocator that works with `mp_Arena`. */
mp_Allocator mp_arena_allocator(const mp_Arena *self);

/* STATIC ARENA ALLOCATOR */
// may return NULL if failed
typedef struct {
    uintptr_t *buf;
    size_t     size;
    size_t     capacity;
} mp_SArena;

/* Initializes and allocates a static arena. `capacity` in words. */
void mp_sarena_init(mp_SArena *self, size_t capacity);
/* Resets the size of the arena. */
void mp_sarena_reset(mp_SArena *self);
/* Frees the arena. */
void mp_sarena_destroy(mp_SArena *self);
/* Returns an allocator that works with `mp_SArena`. */
mp_Allocator mp_sarena_allocator(const mp_SArena *self);

/* TEMP ALLOCATOR
 * mp_SArena located in the stack. */
// may return NULL if failed
typedef struct {
    uintptr_t *buf;
    size_t     size;
    size_t     capacity;
} mp_Temp;

/* Declare an array for the use of `mp_Temp`. */
// name: identifier
// size: size in bytes
#define mp_temp_buffer(name, size)                                                                 \
    uintptr_t name[((size) + sizeof(uintptr_t) - 1) / sizeof(uintptr_t)];
/* Initializes a temp allocator with an array as buf. */
#define mp_temp_init(self, buffer) mp_temp_init_size((self), (buffer), sizeof(buffer))
void mp_temp_init_size(mp_Temp *self, void *buffer, size_t capacity);
/* Resets the size of the temp allocator */
void mp_temp_reset(mp_Temp *self);
/* Returns an allocator that works with `mp_Temp`. */
mp_Allocator mp_temp_allocator(const mp_Temp *self);

/* HEAP ALLOCATOR */
// may return NULL if failed

mp_Allocator mp_heap_allocator(void);

/***********
 * END OF ALLOCATOR
 ***********/

/***********
 * STRING
 ***********/

/* Holds a null-terminated string and the size of the string (excluding the null-terminator). */
typedef struct {
    size_t size;
    char  *cstr;
} mp_String;

/* Allocates a new `mp_String` from a null-terminated string. */
mp_String mp_string_new(const mp_Allocator *allocator, const char *str);
/* Allocates a new `mp_String` from formatted input. */
mp_String mp_string_newf(const mp_Allocator *allocator, const char *fmt, ...);
/* Allocates duplicate of `str`. */
mp_String mp_string_dup(const mp_Allocator *allocator, mp_String str);
/* Free an `mp_String`. */
void mp_string_destroy(const mp_Allocator *allocator, mp_String *str);

/***********
 * END OF STRING
 ***********/

/***********
 * VECTOR
 ***********/

/* Starting capacity of a vector. You can adjust this to your liking. */
#ifndef MP_VECTOR_INIT_CAPACITY
#define MP_VECTOR_INIT_CAPACITY 64
#endif

/* You can define a vector struct with any type as long as it's in this format. */
/*
    typedef struct {
        mp_Allocator *alloc;    // The allocator that manages the allocation of the vector
        size_t       size;      // The size of the vector
        size_t       capacity;  // The capacity of the vector
        <type>       *data;     // Pointer to the data (points to the first element)
        // The data is continuous in memory.
    } Vector;
*/

/* Defines a compatible vector struct given `name` and the data `type`. */
// name: identifier
// type: typename
#define mp_vector_create(name, type)                                                               \
    typedef struct {                                                                               \
        mp_Allocator *alloc;                                                                       \
        size_t        size;                                                                        \
        size_t        capacity;                                                                    \
        type         *data;                                                                        \
    } name

/* Initializes a new vector and tell it to use `allocator`. */
// self: Vector*
// allocator: mp_Allocator*
#define mp_vector_init(self, allocator)                                                            \
    do {                                                                                           \
        (self)->alloc    = (allocator);                                                            \
        (self)->size     = 0;                                                                      \
        (self)->capacity = 0;                                                                      \
        (self)->data     = NULL;                                                                   \
    } while (0)

/* Frees the vector. */
// self: Vector*
#define mp_vector_destroy(self)                                                                    \
    do {                                                                                           \
        mp_free((self)->alloc, (self)->data);                                                      \
        (self)->alloc    = NULL;                                                                   \
        (self)->size     = 0;                                                                      \
        (self)->capacity = 0;                                                                      \
        (self)->data     = NULL;                                                                   \
    } while (0)

// TOOO: make these functions apply to other data structures later

/* Gets an item at index `i`. */
// self: Vector
// i: size_t
#define mp_get(self, i) (self).data[i]

/* Resizes vector to `offset` of the current size.
 * If the current capacity is 0, allocates for `MP_VECTOR_INIT_CAPACITY` items.
 * If the current capacity is not large enough, allocates for double the current capacity.
 * self.data == NULL and self.capacity == -1 if allocation failed.
 * Positive `offset` grows the vector.
 * Negative `offset` shrinks the vector. */
// self: Vector*
// offset: int
#define mp_resize(self, offset)                                                                    \
    do {                                                                                           \
        if ((self)->size + (offset) > (self)->capacity && (offset) > 0) {                          \
            size_t old_capacity = (self)->capacity;                                                \
            if ((self)->capacity == 0) {                                                           \
                (self)->capacity = MP_VECTOR_INIT_CAPACITY;                                        \
            }                                                                                      \
            while ((self)->size + (offset) > (self)->capacity) {                                   \
                (self)->capacity *= 2;                                                             \
            }                                                                                      \
            (self)->data = mp_realloc((self)->alloc,                                               \
                                      (self)->data,                                                \
                                      old_capacity * sizeof(*(self)->data),                        \
                                      (self)->capacity * sizeof(*(self)->data));                   \
        }                                                                                          \
        if ((self)->data != NULL)                                                                  \
            (self)->size += (offset);                                                              \
        else                                                                                       \
            (self)->capacity = -1;                                                                 \
    } while (0)

/* Changes the capacity of the vector.
 * Shrinks the vector `capacity` is smaller than the current size.
 * Reallocate the vector if `capacity` is larger than the current capacity.
 * self.data == NULL and self.capacity == -1 if allocation failed. */
// self: Vector*
// new_capacity: size_t
#define mp_reserve(self, new_capacity)                                                             \
    do {                                                                                           \
        if ((new_capacity) < (self)->size) {                                                       \
            mp_resize((self), (new_capacity) - (self)->size);                                      \
        } else if ((new_capacity) > (self)->capacity) {                                            \
            (self)->data = mp_realloc((self)->alloc,                                               \
                                      (self)->data,                                                \
                                      (self)->capacity * sizeof(*(self)->data),                    \
                                      (new_capacity) * sizeof(*(self)->data));                     \
        }                                                                                          \
        if ((self)->data != NULL)                                                                  \
            (self)->capacity = (new_capacity);                                                     \
        else                                                                                       \
            (self)->capacity = -1;                                                                 \
    } while (0)

/* Resizes the vector and appends item to the end. */
// self: Vector*
// item: value of the same type as the vector data
#define mp_append(self, item)                                                                      \
    do {                                                                                           \
        mp_resize(self, 1);                                                                        \
        (self)->data[(self)->size - 1] = (item);                                                   \
    } while (0)

/* Resizes the vector and appends items from `items_ptr` to the end. */
// self: Vector*
// items_ptr: pointer to the same type as the vector data
// items_amount: size_t
#define mp_append_many(self, items_ptr, items_amount)                                              \
    do {                                                                                           \
        mp_resize((self), (items_amount));                                                         \
        memcpy((self)->data + ((self)->size - (items_amount)),                                     \
               (items_ptr),                                                                        \
               (items_amount) * sizeof(*(self)->data));                                            \
    } while (0)

/* Clones the vector to `dest` to be managed by `allocator`. */
// self: Vector*
// dest: Vector*
// allocator: mp_Allocator*
#define mp_clone(self, dest, allocator)                                                            \
    do {                                                                                           \
        (dest)->alloc    = (allocator);                                                            \
        (dest)->size     = (self)->size;                                                           \
        (dest)->capacity = (self)->size + MP_VECTOR_INIT_CAPACITY;                                 \
        (dest)->data =                                                                             \
            mp_dup((allocator), (self)->data, (self)->capacity * sizeof(*(self)->data));           \
    } while (0)

/* Gets the first or the last item in the vector. */
// self: Vector*
#define mp_first(self) (self)->data[0]
#define mp_last(self)  (self)->data[(self)->size - 1]

/* Deletes the last item in the vector and returns it. */
// self: Vector*
#define mp_pop(self) (--(self)->size, (self)->data[(self)->size])

/* Sets the vector size to 0. */
// self: Vector*
#define mp_clear(self)                                                                             \
    do {                                                                                           \
        (self)->size = 0;                                                                          \
    } while (0)

/* Inserts an item to the given `pos`. */
// self: Vector*
// pos: size_t
// item: value of the same type of the vector data
#define mp_insert(self, pos, item)                                                                 \
    do {                                                                                           \
        size_t actual_pos = (pos) > (self)->size ? (self)->size : (pos);                           \
        mp_resize((self), 1);                                                                      \
        for (int i = (self)->size - 2; i > actual_pos; --i)                                        \
            (self)->data[i + 1] = (self)->data[i];                                                 \
        (self)->data[actual_pos + 1] = (self)->data[actual_pos];                                   \
        (self)->data[actual_pos]     = (item);                                                     \
    } while (0)

/* Inserts items from `items_ptr` to the given `pos`.
 * Items previously at and after `pos` are shifted. */
// self: Vector*
// pos: size_t
// items_ptr: pointer to the same type as the vector data
// items_amount: size_t
#define mp_insert_many(self, pos, items_ptr, amount)                                               \
    do {                                                                                           \
        size_t actual_pos = (pos) > (self)->size ? (self)->size : (pos);                           \
        mp_resize((self), (amount));                                                               \
        for (int i = (self)->size - 1 - (amount); i > actual_pos; --i)                             \
            (self)->data[i + amount] = (self)->data[i];                                            \
        (self)->data[actual_pos + amount] = (self)->data[actual_pos];                              \
        memcpy((self)->data + actual_pos, (items_ptr), (amount) * sizeof(*(self)->data));          \
    } while (0)

/* Deletes an item at the given `pos`. */
// self: Vector*
// pos: size_t
#define mp_erase(self, pos)                                                                        \
    do {                                                                                           \
        _MEMPLUS_ASSERT((pos) < (self)->size && "index out of bounds");                            \
        mp_resize((self), -1);                                                                     \
        for (size_t i = (pos) + 1; i < (self)->size + 1; ++i)                                      \
            (self)->data[i - 1] = (self)->data[i];                                                 \
    } while (0)

/* Deletes an item at the given `pos` and return that item. */
// self: Vector*
// pos: size_t
#define mp_erase_ret(self, pos)                                                                    \
    (self)->data[pos];                                                                             \
    mp_erase((self), (pos))

/* Deletes items from `items_ptr` at the given `pos`.
 * Items are shifted to fill the spaces left by deleted items. */
// self: Vector*
// pos: size_t
// amount: size_t
#define mp_erase_many(self, pos, amount)                                                           \
    do {                                                                                           \
        _MEMPLUS_ASSERT((pos) + (amount) <= (self)->size && "index out of bounds");                \
        mp_resize((self), -(amount));                                                              \
        for (size_t i = (pos) + (amount); i < (self)->size + (amount); ++i)                        \
            (self)->data[i - (amount)] = (self)->data[i];                                          \
    } while (0)

/* Same as `mp_erase_many`, but also writes the deleted items to `buf`. */
// self: Vector*
// pos: size_t
// buf: pointer to a buffer containing the same type as the vector data
// amount: size_t
#define mp_erase_many_to_buf(self, pos, buf, amount)                                               \
    do {                                                                                           \
        _MEMPLUS_ASSERT((pos) + (amount) <= (self)->size && "index out of bounds");                \
        mp_resize((self), -(amount));                                                              \
        for (size_t i = 0; i < (amount); ++i)                                                      \
            (buf)[i] = (self)->data[(pos) + i];                                                    \
        for (size_t i = (pos) + (amount); i < (self)->size + (amount); ++i)                        \
            (self)->data[i - (amount)] = (self)->data[i];                                          \
    } while (0)

/* Deletes an item at the given `pos`. This operation is O(1). */
// self: Vector*
// pos: size_t
#define mp_unordered_erase(self, pos)                                                              \
    do {                                                                                           \
        _MEMPLUS_ASSERT((pos) < (self)->size && "index out of bounds");                            \
        mp_resize((self), -1);                                                                     \
        if ((pos) != (self)->size) (self)->data[pos] = (self)->data[(self)->size];                 \
    } while (0)

/* Deletes an item at the given `pos` and return that item. */
// self: Vector*
// pos: size_t
#define mp_unordered_erase_ret(self, pos)                                                          \
    (self)->data[pos];                                                                             \
    mp_unordered_erase((self), (pos))

/***********
 * END OF VECTOR
 ***********/

/**********
 * MISCELLANEOUS
 **********/

/* Reads and allocates the content in `file_path` and return it to `output`.
 * Returns false if failed and sets errno through stdlib functions. */
bool mp_read_entire_file(mp_Allocator *allocator, mp_String *output, const char *file_path);

/**********
 * END OF MISCELLANEOUS
 **********/

/***********
 * IMPLEMENTATION
 ***********/

#ifdef MEMPLUS_IMPLEMENTATION

/* Functions that are used by `mp_*_new_allocator` to define the allocator. */
static void *mp_arena_alloc(mp_Arena *self, size_t size);
static void *mp_arena_realloc(mp_Arena *self, void *old_ptr, size_t old_size, size_t new_size);
static void *mp_arena_dup(mp_Arena *self, void *data, size_t size);
static void  mp_arena_free(mp_Arena *self, void *ptr);

static void *mp_sarena_alloc(mp_SArena *self, size_t size);
static void *mp_sarena_realloc(mp_SArena *self, void *old_ptr, size_t old_size, size_t new_size);
static void *mp_sarena_dup(mp_SArena *self, void *data, size_t size);
static void  mp_sarena_free(mp_SArena *self, void *ptr);

static void *mp_heap_alloc(void *self, size_t size);
static void *mp_heap_realloc(void *self, void *old_ptr, size_t old_size, size_t new_size);
static void *mp_heap_dup(void *self, void *data, size_t size);
static void  mp_heap_free(void *self, void *ptr);

mp_Region *mp_region_new(size_t capacity) {
    size_t     bytes  = sizeof(mp_Region) + sizeof(uintptr_t) * capacity;
    mp_Region *region = calloc(bytes, 1);
    _MEMPLUS_ASSERT(region != NULL);
    region->next     = NULL;
    region->size     = 0;
    region->capacity = capacity;
    return region;
}

void mp_region_free(mp_Region *self) {
    free(self);
}

void mp_arena_init(mp_Arena *self) {
    self->size  = 0;
    self->begin = NULL;
    self->end   = NULL;
}

void mp_arena_destroy(mp_Arena *self) {
    mp_Region *region = self->begin;
    while (region) {
        mp_Region *region_temp = region;
        region                 = region->next;
        mp_region_free(region_temp);
    }
    self->begin = NULL;
    self->end   = NULL;
}

mp_Allocator mp_arena_allocator(const mp_Arena *self) {
    return mp_allocator_new(self, mp_arena_alloc, mp_arena_realloc, mp_arena_dup, mp_arena_free);
}

static void *mp_arena_alloc(mp_Arena *self, size_t size) {
    // size in words
    size_t size_word = (size + sizeof(uintptr_t) - 1) / sizeof(uintptr_t);

    if (self->end == NULL) {
        _MEMPLUS_ASSERT(self->begin == NULL);
        size_t capacity = MP_REGION_DEFAULT_SIZE;
        if (capacity < size_word) capacity = size_word;
        self->end   = mp_region_new(capacity);
        self->begin = self->end;
    }

    while (self->end->size + size_word > self->end->capacity && self->end->next != NULL) {
        self->end = self->end->next;
    }

    if (self->end->size + size_word > self->end->capacity) {
        _MEMPLUS_ASSERT(self->end->next == NULL);
        size_t capacity = MP_REGION_DEFAULT_SIZE;
        if (capacity < size_word) capacity = size_word;
        self->end->next = mp_region_new(capacity);
        self->end       = self->end->next;
    }

    void *result = &self->end->data[self->end->size];
    self->end->size += size_word;
    self->size += size_word;
    return result;
}

static void *mp_arena_realloc(mp_Arena *self, void *old_ptr, size_t old_size, size_t new_size) {
    if (new_size <= old_size) return old_ptr;
    void    *new_ptr      = mp_arena_alloc(self, new_size);
    uint8_t *new_ptr_byte = (uint8_t *) new_ptr;
    uint8_t *old_ptr_byte = (uint8_t *) old_ptr;
    for (size_t i = 0; i < old_size; ++i) {
        new_ptr_byte[i] = old_ptr_byte[i];
    }
    return new_ptr;
}

static void *mp_arena_dup(mp_Arena *self, void *data, size_t size) {
    return memcpy(mp_arena_alloc(self, size), data, size);
}

static void mp_arena_free(mp_Arena *self, void *ptr) {
    (void) self, (void) ptr;
}

void mp_sarena_init(mp_SArena *self, size_t capacity) {
    uintptr_t *buffer = calloc(capacity * sizeof(uintptr_t), 1);
    self->buf         = buffer;
    self->size        = 0;
    self->capacity    = capacity;
}

void mp_sarena_reset(mp_SArena *self) {
    memset(self->buf, 0, self->capacity);
    self->size = 0;
}

void mp_sarena_destroy(mp_SArena *self) {
    free(self->buf);
    self->buf      = NULL;
    self->size     = 0;
    self->capacity = 0;
}

mp_Allocator mp_sarena_allocator(const mp_SArena *self) {
    return mp_allocator_new(
        self, mp_sarena_alloc, mp_sarena_realloc, mp_sarena_dup, mp_sarena_free);
}

static void *mp_sarena_alloc(mp_SArena *self, size_t size) {
    size_t size_word = (size + sizeof(uintptr_t) - 1) / sizeof(uintptr_t);
    if (self->size + size_word > self->capacity) return NULL;
    void *result = &self->buf[self->size];
    self->size += size_word;
    return result;
}

static void *mp_sarena_realloc(mp_SArena *self, void *old_ptr, size_t old_size, size_t new_size) {
    if (new_size <= old_size) return old_ptr;
    void *new_ptr = mp_sarena_alloc(self, new_size);
    if (new_ptr == NULL) return NULL;
    uint8_t *new_ptr_byte = (uint8_t *) new_ptr;
    uint8_t *old_ptr_byte = (uint8_t *) old_ptr;
    for (size_t i = 0; i < old_size; ++i) {
        new_ptr_byte[i] = old_ptr_byte[i];
    }
    return new_ptr;
}

static void *mp_sarena_dup(mp_SArena *self, void *data, size_t size) {
    void *buf = mp_sarena_alloc(self, size);
    if (buf == NULL) return NULL;
    return memcpy(buf, data, size);
}

static void mp_sarena_free(mp_SArena *self, void *ptr) {
    (void) self, (void) ptr;
}

void mp_temp_init_size(mp_Temp *self, void *buffer, size_t capacity) {
    memset(buffer, 0, capacity);
    self->buf      = buffer;
    self->size     = 0;
    self->capacity = capacity / sizeof(uintptr_t);
}

void mp_temp_reset(mp_Temp *self) {
    memset(self->buf, 0, self->capacity);
    self->size = 0;
}

mp_Allocator mp_temp_allocator(const mp_Temp *self) {
    return mp_allocator_new(
        self, mp_sarena_alloc, mp_sarena_realloc, mp_sarena_dup, mp_sarena_free);
}

mp_Allocator mp_heap_allocator(void) {
    return mp_allocator_new(NULL, mp_heap_alloc, mp_heap_realloc, mp_heap_dup, mp_heap_free);
}

static void *mp_heap_alloc(void *self, size_t size) {
    (void) self;
    return calloc(size, 1);
}

static void *mp_heap_realloc(void *self, void *old_ptr, size_t old_size, size_t new_size) {
    (void) self;
    if (new_size <= old_size) return old_ptr;
    return realloc(old_ptr, new_size);
}

static void *mp_heap_dup(void *self, void *data, size_t size) {
    (void) self;
    void *buf = mp_heap_alloc(self, size);
    if (buf == NULL) return NULL;
    return memcpy(buf, data, size);
}

static void mp_heap_free(void *self, void *ptr) {
    (void) self;
    free(ptr);
}

mp_String mp_string_new(const mp_Allocator *allocator, const char *str) {
    int size = snprintf(NULL, 0, "%s", str);
    _MEMPLUS_ASSERT(size >= 0 && "failed to count string size");
    char *result      = mp_alloc(allocator, size + 1);
    int   result_size = snprintf(result, size + 1, "%s", str);
    _MEMPLUS_ASSERT(result_size == size);
    return (mp_String){ result_size, result };
}

mp_String mp_string_newf(const mp_Allocator *allocator, const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    int size = vsnprintf(NULL, 0, fmt, args);
    _MEMPLUS_ASSERT(size >= 0 && "failed to count string size");
    va_end(args);

    char *result = mp_alloc(allocator, size + 1);

    va_start(args, fmt);
    int result_size = vsnprintf(result, size + 1, fmt, args);
    _MEMPLUS_ASSERT(result_size == size);
    va_end(args);

    return (mp_String){ result_size, result };
}

mp_String mp_string_dup(const mp_Allocator *allocator, mp_String str) {
    int size = snprintf(NULL, 0, "%s", str.cstr);
    _MEMPLUS_ASSERT((size >= 0 || (size_t) size != str.size) && "failed to count string size");
    char *ptr = mp_dup(allocator, str.cstr, size);
    return (mp_String){ size, ptr };
}

void mp_string_destroy(const mp_Allocator *allocator, mp_String *str) {
    mp_free(allocator, str->cstr);
    str->size = 0;
}

bool mp_read_entire_file(mp_Allocator *allocator, mp_String *output, const char *file_path) {
    bool         result     = true;
    mp_Allocator heap_alloc = mp_heap_allocator();
    char        *buffer     = NULL;

    FILE *file = fopen(file_path, "r");
    if (file == NULL) return false;

    if (fseek(file, 0, SEEK_END) < 0) return_defer(false);
    long file_size = ftell(file);
    if (file_size < 0) return_defer(false);
    if (fseek(file, 0, SEEK_SET) < 0) return_defer(false);
    buffer = mp_alloc(&heap_alloc, file_size + 1);
    if (buffer == NULL) return_defer(false);
    long bytes_read = fread(buffer, 1, file_size, file);
    if (bytes_read != file_size || ferror(file) != 0) return_defer(false);
    buffer[file_size] = '\0';
    *output           = mp_string_new(allocator, buffer);

defer:
    mp_free(&heap_alloc, buffer);
    fclose(file);
    return result;
}

#endif /* ifdef MEMPLUS_IMPLEMENTATION */

/***********
 * END OF IMPLEMENTATION
 ***********/

#endif /* ifndef MEMPLUS_H__ */
