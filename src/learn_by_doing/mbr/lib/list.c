#include "global.h"
#include "interrupt.h"
#include "list.h"


void list_init(struct list* list) {
    list->head.prev = NULL;
    list->head.next = &list->tail;
    list->tail.next = NULL;
    list->tail.prev = &list->head;
}

void list_insert_before(struct list_elem* before, struct list_elem *elem) {
    enum intr_status old_status = intr_disable();
    elem->next = before;
    elem->prev = before->prev;
    before->prev->next = elem;
    before->prev = elem;
    intr_set_status(old_status);
}

void list_push(struct list* plist, struct list_elem* elem) {
    list_insert_before(plist->head.next, elem);
}

void list_iterate(struct list* plist) {

}

void list_append(struct list* plist, struct list_elem* elem) {
    list_insert_before(&plist->tail, elem);
}

void list_remove(struct list_elem* elem) {
    enum intr_status old_status = intr_disable();
    elem->next->prev = elem->prev;
    elem->prev->next = elem->next;
    intr_set_status(old_status);
}

struct list_elem* list_pop(struct list* plist) {
    struct list_elem* elem = plist->head.next;
    list_remove(elem);
    return elem;
}

bool list_empty(struct list* plist) {
    return (plist->head.next == &plist->tail ? true: false);
}

uint32_t list_len(struct list* plist) {
    uint32_t len = 0;
    struct list_elem* elem = plist->head.next;
    while(elem != &plist->tail) {
        len++;
        elem = elem->next;
    }
    return len;
}

struct list_elem* list_traversal(struct list* plist, function func, int arg) {
    struct list_elem* elem = plist->head.next;

    if(list_empty(plist)) return NULL;

    while(elem != &plist->tail) {
        if(func(elem, arg)) return elem;
        else elem = elem->next;
    }
    return NULL;
}

bool elem_find(struct list* plist, struct list_elem* obj_elem) {
    struct list_elem* elem = plist->head.next;
    while(elem != &plist->tail) {
        if(elem == obj_elem) return true;
        else elem = elem->next;
    }
    return false;
}

