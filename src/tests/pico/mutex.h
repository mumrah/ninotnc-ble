#include <stdbool.h>
#include "munit.h"

typedef struct {
    bool locked;
} mutex_t;

void mutex_init(mutex_t *mt) {
    mt->locked = false;
}

void mutex_enter_blocking(mutex_t *mt) {
    mt->locked = true;
}

bool mutex_try_enter(mutex_t *mt, void* owner) {
    UNUSED(owner);
    mt->locked = true;
    return true;
}

void mutex_exit(mutex_t *mt) {
    mt->locked = false;
}
