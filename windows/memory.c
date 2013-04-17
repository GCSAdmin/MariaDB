#ident "Copyright (c) 2007-2010 Tokutek Inc.  All rights reserved."

#include <toku_portability.h>
#include "memory.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "toku_assert.h"

int toku_memory_check=0;

static malloc_fun_t  t_malloc  = 0;
static malloc_fun_t  t_xmalloc = 0;
static free_fun_t    t_free    = 0;
static realloc_fun_t t_realloc = 0;
static realloc_fun_t t_xrealloc = 0;

static MEMORY_STATUS_S status;

void 
toku_memory_get_status(MEMORY_STATUS s) {
    *s = status;
}



void *toku_malloc(size_t size) {
    void *p = t_malloc ? t_malloc(size) : os_malloc(size);
    size_t used = malloc_usable_size(p);
    __sync_add_and_fetch(&status.malloc_count, 1L);
    __sync_add_and_fetch(&status.requested, size);
    __sync_add_and_fetch(&status.used, used);
    return p;
}

void *
toku_calloc(size_t nmemb, size_t size) {
    size_t newsize = nmemb * size;
    void *p = toku_malloc(newsize);
    if (p) memset(p, 0, newsize);
    return p;
}

void *
toku_realloc(void *p, size_t size) {
    size_t used_orig = malloc_usable_size(p);
    void *q = t_realloc ? t_realloc(p, size) : os_realloc(p, size);
    size_t used = malloc_usable_size(q);
    __sync_add_and_fetch(&status.realloc_count, 1L);
    __sync_add_and_fetch(&status.requested, size);
    __sync_add_and_fetch(&status.used, used);
    __sync_add_and_fetch(&status.freed, used_orig);
    return q;
}

void *
toku_memdup(const void *v, size_t len) {
    void *p = toku_malloc(len);
    if (p) memcpy(p, v,len);
    return p;
}

char *
toku_strdup(const char *s) {
    return toku_memdup(s, strlen(s)+1);
}

void
toku_free(void *p) {
    size_t used = malloc_usable_size(p);
    __sync_add_and_fetch(&status.free_count, 1L);
    __sync_add_and_fetch(&status.freed, used);
    if (t_free)
	t_free(p);
    else
	os_free(p);
}

void
toku_free_n(void* p, size_t size __attribute__((unused))) {
    toku_free(p);
}

void *
toku_xmalloc(size_t size) {
    void *p = t_xmalloc ? t_xmalloc(size) : os_malloc(size);
    if (p == NULL)  // avoid function call in common case
        resource_assert(p);
    size_t used = malloc_usable_size(p);
    __sync_add_and_fetch(&status.malloc_count, 1L);
    __sync_add_and_fetch(&status.requested, size);
    __sync_add_and_fetch(&status.used, used);
    return p;
}

void *
toku_xcalloc(size_t nmemb, size_t size) {
    size_t newsize = nmemb * size;
    void *vp = toku_xmalloc(newsize);
    if (vp) memset(vp, 0, newsize);
    return vp;
}

void *
toku_xrealloc(void *v, size_t size) {
    size_t used_orig = malloc_usable_size(v);
    void *p = t_xrealloc ? t_xrealloc(v, size) : os_realloc(v, size);
    size_t used = malloc_usable_size(p);
    if (p == 0)  // avoid function call in common case
        resource_assert(p);
    __sync_add_and_fetch(&status.realloc_count, 1L);
    __sync_add_and_fetch(&status.requested, size);
    __sync_add_and_fetch(&status.used, used);
    __sync_add_and_fetch(&status.freed, used_orig);
    return p;
}

void *
toku_xmemdup (const void *v, size_t len) {
    void *p = toku_xmalloc(len);
    memcpy(p, v, len);
    return p;
}

char *
toku_xstrdup (const char *s) {
    return toku_xmemdup(s, strlen(s)+1);
}

void
toku_set_func_malloc(malloc_fun_t f) {
    t_malloc = f;
    t_xmalloc = f;
}

void
toku_set_func_xmalloc_only(malloc_fun_t f) {
    t_xmalloc = f;
}

void
toku_set_func_malloc_only(malloc_fun_t f) {
    t_malloc = f;
}

void
toku_set_func_realloc(realloc_fun_t f) {
    t_realloc = f;
    t_xrealloc = f;
}

void
toku_set_func_xrealloc_only(realloc_fun_t f) {
    t_xrealloc = f;
}

void
toku_set_func_realloc_only(realloc_fun_t f) {
    t_realloc = f;

}

void
toku_set_func_free(free_fun_t f) {
    t_free = f;
}

