#include "indicator.h"
#include "main.h"

#include <stddef.h>
#include <stdint.h>

typedef struct {
    GPIO_PinState state;
    uint16_t duration_ms;
} indicator_step_t;

typedef struct {
    const indicator_step_t *steps;
    uint8_t step_count;
    bool repeat;
} indicator_pattern_t;

static const indicator_step_t peripherals_ready_steps[] = {
    {GPIO_PIN_SET, 180U},
    {GPIO_PIN_RESET, 120U}
};

static const indicator_step_t detection_steps[] = {
    {GPIO_PIN_SET, 100U},
    {GPIO_PIN_RESET, 100U},
    {GPIO_PIN_SET, 100U},
    {GPIO_PIN_RESET, 100U},
    {GPIO_PIN_SET, 100U},
    {GPIO_PIN_RESET, 100U}
};

static const indicator_step_t error_steps[] = {
    {GPIO_PIN_SET, 600U},
    {GPIO_PIN_RESET, 250U},
    {GPIO_PIN_SET, 600U},
    {GPIO_PIN_RESET, 800U}
};

static const indicator_pattern_t patterns[INDICATION_COUNT] = {
    [INDICATION_PERIPHERALS_READY] = {
        peripherals_ready_steps,
        (uint8_t)(sizeof(peripherals_ready_steps) / sizeof(peripherals_ready_steps[0])),
        false
    },
    [INDICATION_DETECTION] = {
        detection_steps,
        (uint8_t)(sizeof(detection_steps) / sizeof(detection_steps[0])),
        false
    },
    [INDICATION_ERROR] = {
        error_steps,
        (uint8_t)(sizeof(error_steps) / sizeof(error_steps[0])),
        true
    }
};

static const indicator_pattern_t *active_pattern;
static uint8_t active_step;
static uint32_t step_started_at;

static void set_led(GPIO_PinState state)
{
    HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, state);
}

bool indicator_request(indicator_event_t event)
{
    if ((unsigned int)event >= (unsigned int)INDICATION_COUNT) {
        return false;
    }

    const indicator_pattern_t *pattern = &patterns[event];
    if ((pattern->steps == NULL) || (pattern->step_count == 0U)) {
        return false;
    }

    active_pattern = pattern;
    active_step = 0U;
    step_started_at = HAL_GetTick();
    set_led(active_pattern->steps[active_step].state);
    return true;
}

void indicator_update(void)
{
    if (active_pattern == NULL) {
        return;
    }

    uint32_t now = HAL_GetTick();
    uint16_t duration = active_pattern->steps[active_step].duration_ms;
    if ((uint32_t)(now - step_started_at) < duration) {
        return;
    }

    active_step++;
    if (active_step >= active_pattern->step_count) {
        if (!active_pattern->repeat) {
            active_pattern = NULL;
            set_led(GPIO_PIN_RESET);
            return;
        }
        active_step = 0U;
    }

    step_started_at = now;
    set_led(active_pattern->steps[active_step].state);
}
