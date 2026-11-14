#include "app.h"
#include "indicator.h"
#include "main.h"

#include <stdbool.h>
#include <stdint.h>

#define AUDIO_DMA_SAMPLE_COUNT 160U
#define AUDIO_BLOCK_SAMPLE_COUNT (AUDIO_DMA_SAMPLE_COUNT / 2U)
#define AUDIO_BLOCK_BYTES (AUDIO_BLOCK_SAMPLE_COUNT * sizeof(int16_t))
#define AUDIO_BLOCK_HALF 0x01U
#define AUDIO_BLOCK_FULL 0x02U

extern DFSDM_Filter_HandleTypeDef hdfsdm1_filter0;
extern UART_HandleTypeDef huart3;

static int32_t dfsdm_dma[AUDIO_DMA_SAMPLE_COUNT];
static uint8_t uart_tx_buffer[AUDIO_BLOCK_BYTES];

static volatile uint8_t ready_blocks;
static volatile bool uart_tx_busy;
static volatile bool audio_fault;

volatile uint32_t audio_capture_blocks;
volatile uint32_t audio_capture_overruns;
volatile uint32_t audio_uart_blocks;
volatile uint32_t audio_uart_drops;
volatile uint32_t audio_uart_errors;
volatile uint32_t audio_dfsdm_errors;

static void mark_block_ready(uint8_t block) {
    if ((ready_blocks & block) != 0U) {
        audio_capture_overruns++;
    }
    else {
        ready_blocks |= block;
        audio_capture_blocks++;
    }
}

static bool take_ready_block(uint8_t *block) {
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    uint8_t pending = ready_blocks;
    if ((pending & AUDIO_BLOCK_HALF) != 0U) {
        *block = AUDIO_BLOCK_HALF;
        ready_blocks &= (uint8_t)~AUDIO_BLOCK_HALF;
    }
    else if ((pending & AUDIO_BLOCK_FULL) != 0U) {
        *block = AUDIO_BLOCK_FULL;
        ready_blocks &= (uint8_t)~AUDIO_BLOCK_FULL;
    }
    else {
        __set_PRIMASK(primask);
        return false;
    }

    __set_PRIMASK(primask);
    return true;
}

static void transmit_block(uint8_t block) {
    if (uart_tx_busy) {
        audio_uart_drops++;
        return;
    }

    uart_tx_busy = true;
    uint32_t offset = (block == AUDIO_BLOCK_FULL) ? AUDIO_BLOCK_SAMPLE_COUNT : 0U;

    for (uint32_t i = 0U; i < AUDIO_BLOCK_SAMPLE_COUNT; i++) {
        int32_t sample = dfsdm_dma[offset + i] >> 8;
        int16_t pcm_sample = (int16_t)sample;
        uart_tx_buffer[2U * i] = (uint8_t)((uint16_t)pcm_sample & 0xFFU);
        uart_tx_buffer[2U * i + 1U] = (uint8_t)(((uint16_t)pcm_sample >> 8) & 0xFFU);
    }

    __DMB();
    if (HAL_UART_Transmit_DMA(&huart3, uart_tx_buffer, (uint16_t)AUDIO_BLOCK_BYTES) != HAL_OK) {
        uart_tx_busy = false;
        audio_uart_errors++;
        audio_fault = true;
        return;
    }

    audio_uart_blocks++;
}

void app_run(void) {
    indicator_update();

    if (HAL_DFSDM_FilterRegularStart_DMA(&hdfsdm1_filter0, dfsdm_dma,AUDIO_DMA_SAMPLE_COUNT) != HAL_OK) {
        Error_Handler();
    }

    while (true) {
        indicator_update();

        if (audio_fault) {
            Error_Handler();
        }

        uint8_t block;
        if (take_ready_block(&block)) {
            transmit_block(block);
        }
    }
}

void HAL_DFSDM_FilterRegConvHalfCpltCallback(DFSDM_Filter_HandleTypeDef *filter) {
    if (filter == &hdfsdm1_filter0) {
        mark_block_ready(AUDIO_BLOCK_HALF);
    }
}

void HAL_DFSDM_FilterRegConvCpltCallback(DFSDM_Filter_HandleTypeDef *filter) {
    if (filter == &hdfsdm1_filter0) {
        mark_block_ready(AUDIO_BLOCK_FULL);
    }
}

void HAL_DFSDM_FilterErrorCallback(DFSDM_Filter_HandleTypeDef *filter) {
    if (filter == &hdfsdm1_filter0) {
        audio_dfsdm_errors++;
        audio_fault = true;
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *uart) {
    if (uart == &huart3) {
        uart_tx_busy = false;
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *uart) {
    if (uart == &huart3) {
        audio_uart_errors++;
        audio_fault = true;
    }
}
