#pragma once

// LLL: Linked List Library (proud of that acronym)

// The user never "sees" the ll_nodeattr struct
#include "types.h"

typedef struct ll_head {
    void * start; // a ll_nodeattr-struct
    uint32_t magic; // LL_HEADMAGIC
    size_t node_sz;
} ll_head;

// C MADE ME DO IT C MADE ME DO IT C MADE ME DO IT
typedef struct ll_nodeattr {
    uint32_t magic; // LL_NODEMAGIC
    void * next; // a ll_nodeattr-struct
} ll_nodeattr;

#define LL_HEADMAGIC 0x11EADA1C
#define LL_NODEMAGIC 0x110DEA1C

ll_head * create_ll(size_t node_sz);
void destroy_ll(ll_head * head);

size_t ll_len(ll_head * head);
void * ll_push(ll_head * head);
void * ll_get(ll_head * head, size_t index);
int ll_del(ll_head * head, size_t index);
int ll_delp(ll_head * head, void * p);
void * ll_next(void * current);
void * ll_nextla(ll_head * head, void * current);
ll_head * ll_copy(ll_head * head);
