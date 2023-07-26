#include "ll.h"
#include "util.h"
#include "mm.h"

ll_head * create_ll(size_t node_sz) {
    ll_head * out = kmalloc(sizeof(ll_head));

    out->start = NULL;
    out->magic = LL_HEADMAGIC;
    out->node_sz = node_sz;

    return out;
}

void destroy_ll(ll_head * head) {
    if (head->magic != LL_HEADMAGIC)
        kwarn(__FILE__,__func__,"invalid head magic");

    ll_nodeattr * current = head->start;
    ll_nodeattr * next = NULL;

    while (current != NULL) {
        if (current->magic != LL_NODEMAGIC)
            kwarn(__FILE__,__func__,"invalid node magic");

        next = current->next;
        kfree(current);
        current = next;
    }

    kfree(head);
}

size_t ll_len(ll_head * head) {
    if (head->magic != LL_HEADMAGIC)
        kwarn(__FILE__,__func__,"invalid head magic");

    size_t out = 0;

    ll_nodeattr * current = head->start;

    while (current != NULL) {
        if (current->magic != LL_NODEMAGIC)
            kwarn(__FILE__,__func__,"invalid node magic");

        out++;
        current = current->next;
    }

    return out;
}

void * ll_push(ll_head * head) {
    // Adds an empty element to the
    // end of the LL and returns it

    if (head->magic != LL_HEADMAGIC)
        kwarn(__FILE__,__func__,"invalid head magic");

    ll_nodeattr * new = kmalloc(sizeof(ll_nodeattr) + head->node_sz);

    ll_nodeattr * current = head->start;
    ll_nodeattr * prev = NULL;

    while (current != NULL) {
        if (current->magic != LL_NODEMAGIC)
            kwarn(__FILE__,__func__,"invalid node magic");

        prev = current;
        current = current->next;
    }

    if (prev == NULL) {
        head->start = new;
    } else {
        prev->next = new;
    }

    new->magic = LL_NODEMAGIC;
    new->next = NULL;

    return (void*)((uint64_t)new + sizeof(ll_nodeattr));
}

void * ll_get(ll_head * head, size_t index) {
    // Returns the element at the given index
    // or NULL if the index is out of bounds

    if (head->magic != LL_HEADMAGIC)
        kwarn(__FILE__,__func__,"invalid head magic");

    ll_nodeattr * current = head->start;

    for (size_t i = 0; i < index; i++) {
        if (current == NULL) {
            return NULL;
        }

        if (current->magic != LL_NODEMAGIC)
            kwarn(__FILE__,__func__,"invalid node magic");

        current = current->next;
    }

    if (current == NULL) {
        return NULL;
    }

    return (void*)((uint64_t)current + sizeof(ll_nodeattr));
}

int ll_del(ll_head * head, size_t index) {
    // Deletes the element at the given index
    // or does nothing if the index is out of bounds

    if (head->magic != LL_HEADMAGIC)
        kwarn(__FILE__,__func__,"invalid head magic");

    ll_nodeattr * current = head->start;
    ll_nodeattr * prev = NULL;

    for (size_t i = 0; i < index; i++) {
        if (current == NULL) {
            return 1;
        }

        if (current->magic != LL_NODEMAGIC)
            kwarn(__FILE__,__func__,"invalid node magic");

        prev = current;
        current = current->next;
    }

    if (current == NULL) {
        return 1;
    }

    if (prev == NULL) {
        head->start = current->next;
    } else {
        prev->next = current->next;
    }

    kfree(current);

    return 0;
}

int ll_delp(ll_head * head, void * p) {
    // Deletes the pointed to element
    // TODO: test

    if (head->magic != LL_HEADMAGIC)
        kwarn(__FILE__,__func__,"invalid head magic");

    p = (void*)((size_t)p - sizeof(ll_nodeattr));

    ll_nodeattr * curr = head->start;
    size_t i = 0;

    while (curr) {
        if (curr->magic != LL_NODEMAGIC)
            kwarn(__FILE__,__func__,"invalid node magic");

        if (curr == p) {
            return ll_del(head, i); // yes, i am that lazy, and yes, that should always return 0
        }

        curr = curr->next;
        i++;
    }

    return 1;
}

void * ll_next(void * current) {
    ll_nodeattr * attr = (void*)((uint64_t)current - sizeof(ll_nodeattr));

    if (attr->magic != LL_NODEMAGIC)
        kwarn(__FILE__,__func__,"invalid node magic");

    if (attr->next == NULL) {
        return NULL;
    }

    return (void*)((uint64_t)attr->next + sizeof(ll_nodeattr));
}

void * ll_nextla(ll_head * head, void * current) {
    // Gives the next element, like ll_next, but with looparound
    if (head->magic != LL_HEADMAGIC)
        kwarn(__FILE__,__func__,"invalid head magic");

    ll_nodeattr * attr = (void*)((uint64_t)current - sizeof(ll_nodeattr));

    if (attr->magic != LL_NODEMAGIC)
        kwarn(__FILE__,__func__,"invalid node magic");

    if (attr->next == NULL) {
        return (void*)((uint64_t)head->start + sizeof(ll_nodeattr));
    }

    return (void*)((uint64_t)attr->next + sizeof(ll_nodeattr));
}

ll_head * ll_copy(ll_head * head) {
    // Copies the given linked list
    if (head->magic != LL_HEADMAGIC)
        kwarn(__FILE__,__func__,"invalid head magic");

    ll_head * out = create_ll(head->node_sz);

    for (size_t i = 0; i < ll_len(head); i++) {
        void * p = ll_get(head, i);
        void * new = ll_push(out);
        memcpy(new, p, head->node_sz);
    }

    return out;
}
