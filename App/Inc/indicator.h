#ifndef SOUND_RECOGNITION_SYSTEM_INDICATOR_H
#define SOUND_RECOGNITION_SYSTEM_INDICATOR_H

#include <stdbool.h>

typedef enum {
    INDICATION_PERIPHERALS_READY = 0,
    INDICATION_DETECTION,
    INDICATION_ERROR,
    INDICATION_COUNT
} indicator_event_t;

bool indicator_request(indicator_event_t event);
void indicator_update(void);

#endif
