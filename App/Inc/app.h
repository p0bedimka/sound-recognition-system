//
// Created by pobedimka on 14.11.2026.
//

#ifndef SOUND_RECOGNITION_SYSTEM_APP_H
#define SOUND_RECOGNITION_SYSTEM_APP_H

#include <stdint.h>

/* UART3 emits unframed 16 kHz, mono, signed PCM16 little-endian samples. */
void app_run(void);

extern volatile uint32_t audio_capture_blocks;
extern volatile uint32_t audio_capture_overruns;
extern volatile uint32_t audio_uart_blocks;
extern volatile uint32_t audio_uart_drops;
extern volatile uint32_t audio_uart_errors;
extern volatile uint32_t audio_dfsdm_errors;

#endif //SOUND_RECOGNITION_SYSTEM_APP_H
