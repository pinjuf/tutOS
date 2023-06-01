#pragma once

// LLL: Linked List Library (proud of that acronym)

// The user never "sees" the ll_nodeattr struct

#include "types.h"

typedef struct ll_head {
    void * start;
    size_t node_sz;
} ll_head;

// C MADE ME DO IT C MADE ME DO IT C MADE ME DO IT
typedef struct ll_nodeattr ll_nodeattr;
struct ll_nodeattr {
    struct ll_nodeattr * next;
};

ll_head * create_ll(size_t node_sz);
void destroy_ll(ll_head * head);

size_t ll_len(ll_head * head);
void * ll_push(ll_head * head);
void * ll_get(ll_head * head, size_t index);
int ll_del(ll_head * head, size_t index);
void * ll_next(ll_head * head, void * current);
void * ll_nextla(ll_head * head, void * current);
