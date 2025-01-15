#include <stdbool.h>
#include "munit.h"

typedef struct {
    bool locked;
} critical_section_t;

void critical_section_init(critical_section_t *crit_sec) {
    crit_sec->locked = false;
}

void critical_section_enter_blocking(critical_section_t *crit_sec) {
    crit_sec->locked = true;
}

void critical_section_exit(critical_section_t *crit_sec) {
    crit_sec->locked = false;
}
