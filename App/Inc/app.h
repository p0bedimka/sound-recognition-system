//
// Created by pobedimka on 14.11.2026.
//

#ifndef SOUND_RECOGNITION_SYSTEM_APP_H
#define SOUND_RECOGNITION_SYSTEM_APP_H

#include <stdint.h>

/* USART3 receives recording commands and sends framed 16 kHz mono PCM16 audio. */
void app_run(void);

extern volatile uint32_t audio_capture_blocks;
extern volatile uint32_t audio_capture_overruns;
extern volatile uint32_t audio_uart_blocks;
extern volatile uint32_t audio_uart_drops;
extern volatile uint32_t audio_uart_errors;
extern volatile uint32_t audio_dfsdm_errors;
extern volatile uint32_t audio_uart_command_errors;

#endif //SOUND_RECOGNITION_SYSTEM_APP_H
