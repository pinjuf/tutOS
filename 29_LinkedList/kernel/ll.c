#include "ll.h"
#include "util.h"
#include "mm.h"

ll_head * create_ll(size_t node_sz) {
    ll_head * out = kmalloc(sizeof(ll_head));

    out->start = NULL;
    out->node_sz = node_sz;

    return out;
}

void destroy_ll(ll_head * head) {
    ll_nodeattr * current = head->start;
    ll_nodeattr * next = NULL;

    while (current != NULL) {
        next = current->next;
        kfree(current);
        current = next;
    }

    kfree(head);
}

size_t ll_len(ll_head * head) {
    size_t out = 0;

    ll_nodeattr * current = head->start;

    while (current != NULL) {
        out++;
        current = current->next;
    }

    return out;
}

void * ll_push(ll_head * head) {
    // Adds an empty element to the
    // end of the LL and returns it

    ll_nodeattr * new = kmalloc(sizeof(ll_nodeattr) + head->node_sz);

    ll_nodeattr * current = head->start;
    ll_nodeattr * prev = NULL;

    while (current != NULL) {
        prev = current;
        current = current->next;
    }

    if (prev == NULL) {
        head->start = new;
    } else {
        prev->next = new;
    }

    new->next = NULL;

    return (void*)((uint64_t)new + sizeof(ll_nodeattr));
}

void * ll_get(ll_head * head, size_t index) {
    // Returns the element at the given index
    // or NULL if the index is out of bounds

    ll_nodeattr * current = head->start;

    for (size_t i = 0; i < index; i++) {
        if (current == NULL) {
            return NULL;
        }

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

    ll_nodeattr * current = head->start;
    ll_nodeattr * prev = NULL;

    for (size_t i = 0; i < index; i++) {
        if (current == NULL) {
            return 1;
        }

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

void * ll_next(void * current) {
    ll_nodeattr * attr = (void*)((uint64_t)current - sizeof(ll_nodeattr));

    if (attr->next == NULL) {
        return NULL;
    }

    return attr->next + sizeof(ll_nodeattr);
}

void * ll_nextla(ll_head * head, void * current) {
    // Gives the next element, like ll_next, but with looparound
    ll_nodeattr * attr = (void*)((uint64_t)current - sizeof(ll_nodeattr));

    if (attr->next == NULL) {
        return (void*)((uint64_t)head->start + sizeof(ll_nodeattr));
    }

    return attr->next + sizeof(ll_nodeattr);
}
