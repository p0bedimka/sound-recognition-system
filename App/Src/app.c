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

#define UART_SYNC_0 0xA5U
#define UART_SYNC_1 0x5AU
#define UART_PROTOCOL_VERSION 0x02U
#define UART_PACKET_TYPE_START 0x01U
#define UART_PACKET_TYPE_STOP 0x02U
#define UART_PACKET_TYPE_AUDIO 0x10U
#define UART_HEADER_SIZE 10U
#define UART_CRC_SIZE 4U
#define UART_MAX_PAYLOAD_SIZE AUDIO_BLOCK_BYTES
#define UART_MAX_FRAME_SIZE (UART_HEADER_SIZE + UART_MAX_PAYLOAD_SIZE + UART_CRC_SIZE)
#define UART_CRC_MAX_INPUT_SIZE (UART_HEADER_SIZE - 2U + UART_MAX_PAYLOAD_SIZE)
#define UART_COMMAND_QUEUE_SIZE 4U

typedef enum {
    UART_PARSE_SYNC_0,
    UART_PARSE_SYNC_1,
    UART_PARSE_FRAME
} uart_parse_state_t;

typedef enum {
    RECORD_COMMAND_START = UART_PACKET_TYPE_START,
    RECORD_COMMAND_STOP = UART_PACKET_TYPE_STOP
} record_command_t;

extern DFSDM_Filter_HandleTypeDef hdfsdm1_filter0;
extern UART_HandleTypeDef huart3;
extern CRC_HandleTypeDef hcrc;

static int32_t dfsdm_dma[AUDIO_DMA_SAMPLE_COUNT];
static uint8_t uart_tx_buffer[UART_MAX_FRAME_SIZE];
static uint8_t uart_rx_byte;
static uint8_t uart_rx_frame[UART_MAX_FRAME_SIZE];
static uint32_t uart_crc_words[UART_CRC_MAX_INPUT_SIZE / sizeof(uint32_t)];
static uint16_t uart_rx_index;
static uint16_t uart_rx_expected_size;
static uart_parse_state_t uart_parse_state;
static volatile record_command_t command_queue[UART_COMMAND_QUEUE_SIZE];
static volatile uint8_t command_queue_head;
static volatile uint8_t command_queue_tail;
static volatile uint8_t ready_blocks;
static volatile uint32_t ready_block_sequences[2];
static volatile bool uart_tx_busy;
static volatile bool recording;
static volatile bool audio_fault;
static uint32_t audio_packet_sequence;

volatile uint32_t audio_capture_blocks;
volatile uint32_t audio_capture_overruns;
volatile uint32_t audio_uart_blocks;
volatile uint32_t audio_uart_drops;
volatile uint32_t audio_uart_errors;
volatile uint32_t audio_dfsdm_errors;
volatile uint32_t audio_uart_command_errors;

static bool calculate_crc32(const uint8_t *data, uint16_t size, uint32_t *crc) {
    if ((size % sizeof(uint32_t)) != 0U || size > UART_CRC_MAX_INPUT_SIZE) {
        return false;
    }

    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    for (uint16_t i = 0U; i < size; i += sizeof(uint32_t)) {
        uart_crc_words[i / sizeof(uint32_t)] = ((uint32_t)data[i] << 24)
                                               | ((uint32_t)data[i + 1U] << 16)
                                               | ((uint32_t)data[i + 2U] << 8)
                                               | (uint32_t)data[i + 3U];
    }

    *crc = HAL_CRC_Calculate(&hcrc, uart_crc_words, size / sizeof(uint32_t));
    __set_PRIMASK(primask);
    return true;
}

static void queue_command(record_command_t command) {
    uint8_t next_head = (uint8_t)((command_queue_head + 1U) % UART_COMMAND_QUEUE_SIZE);
    if (next_head == command_queue_tail) {
        audio_uart_command_errors++;
        return;
    }

    command_queue[command_queue_head] = command;
    __DMB();
    command_queue_head = next_head;
}

static bool take_command(record_command_t *command) {
    if (command_queue_tail == command_queue_head) {
        return false;
    }

    *command = command_queue[command_queue_tail];
    command_queue_tail = (uint8_t)((command_queue_tail + 1U) % UART_COMMAND_QUEUE_SIZE);
    return true;
}

static void reset_uart_parser(void) {
    uart_parse_state = UART_PARSE_SYNC_0;
    uart_rx_index = 0U;
    uart_rx_expected_size = 0U;
}

static void process_uart_frame(void) {
    uint8_t version = uart_rx_frame[2];
    uint8_t packet_type = uart_rx_frame[3];
    uint16_t payload_size = (uint16_t)uart_rx_frame[8] | ((uint16_t)uart_rx_frame[9] << 8);
    uint32_t crc_offset = UART_HEADER_SIZE + payload_size;
    uint32_t expected_crc = (uint32_t)uart_rx_frame[crc_offset]
                            | ((uint32_t)uart_rx_frame[crc_offset + 1U] << 8)
                            | ((uint32_t)uart_rx_frame[crc_offset + 2U] << 16)
                            | ((uint32_t)uart_rx_frame[crc_offset + 3U] << 24);
    uint32_t actual_crc;

    if (version != UART_PROTOCOL_VERSION
        || !calculate_crc32(&uart_rx_frame[2], (uint16_t)(8U + payload_size), &actual_crc)
        || expected_crc != actual_crc) {
        audio_uart_command_errors++;
        return;
    }

    if (packet_type == UART_PACKET_TYPE_START) {
        if (payload_size == 0U) {
            queue_command(RECORD_COMMAND_START);
        } else {
            audio_uart_command_errors++;
        }
    } else if (packet_type == UART_PACKET_TYPE_STOP) {
        if (payload_size == 0U) {
            queue_command(RECORD_COMMAND_STOP);
        } else {
            audio_uart_command_errors++;
        }
    }
}

static void parse_uart_byte(uint8_t byte) {
    if (uart_parse_state == UART_PARSE_SYNC_0) {
        if (byte == UART_SYNC_0) {
            uart_rx_frame[0] = byte;
            uart_parse_state = UART_PARSE_SYNC_1;
        }
        return;
    }

    if (uart_parse_state == UART_PARSE_SYNC_1) {
        if (byte == UART_SYNC_1) {
            uart_rx_frame[1] = byte;
            uart_rx_index = 2U;
            uart_parse_state = UART_PARSE_FRAME;
        } else if (byte != UART_SYNC_0) {
            reset_uart_parser();
        }
        return;
    }

    uart_rx_frame[uart_rx_index++] = byte;
    if (uart_rx_index == UART_HEADER_SIZE) {
        uint16_t payload_size = (uint16_t)uart_rx_frame[8] | ((uint16_t)uart_rx_frame[9] << 8);
        uint8_t packet_type = uart_rx_frame[3];
        bool is_command = packet_type == UART_PACKET_TYPE_START || packet_type == UART_PACKET_TYPE_STOP;
        if (uart_rx_frame[2] != UART_PROTOCOL_VERSION || !is_command || payload_size != 0U) {
            audio_uart_command_errors++;
            reset_uart_parser();
            return;
        }
        uart_rx_expected_size = (uint16_t)(UART_HEADER_SIZE + payload_size + UART_CRC_SIZE);
    }

    if (uart_rx_expected_size != 0U && uart_rx_index == uart_rx_expected_size) {
        process_uart_frame();
        reset_uart_parser();
    }
}

static void mark_block_ready(uint8_t block) {
    if (!recording) {
        return;
    }

    if ((ready_blocks & block) != 0U) {
        audio_capture_overruns++;
    } else {
        audio_capture_blocks++;
    }
    ready_block_sequences[block == AUDIO_BLOCK_FULL ? 1U : 0U] = audio_packet_sequence++;
    ready_blocks |= block;
}

static bool take_ready_block(uint8_t *block, uint32_t *sequence) {
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    uint8_t pending = ready_blocks;
    if ((pending & AUDIO_BLOCK_HALF) != 0U) {
        *block = AUDIO_BLOCK_HALF;
        *sequence = ready_block_sequences[0];
        ready_blocks &= (uint8_t)~AUDIO_BLOCK_HALF;
    } else if ((pending & AUDIO_BLOCK_FULL) != 0U) {
        *block = AUDIO_BLOCK_FULL;
        *sequence = ready_block_sequences[1];
        ready_blocks &= (uint8_t)~AUDIO_BLOCK_FULL;
    } else {
        __set_PRIMASK(primask);
        return false;
    }

    __set_PRIMASK(primask);
    return true;
}

static void transmit_block(uint8_t block, uint32_t sequence) {
    if (uart_tx_busy) {
        audio_uart_drops++;
        return;
    }

    uint32_t offset = (block == AUDIO_BLOCK_FULL) ? AUDIO_BLOCK_SAMPLE_COUNT : 0U;
    uart_tx_buffer[0] = UART_SYNC_0;
    uart_tx_buffer[1] = UART_SYNC_1;
    uart_tx_buffer[2] = UART_PROTOCOL_VERSION;
    uart_tx_buffer[3] = UART_PACKET_TYPE_AUDIO;
    uart_tx_buffer[4] = (uint8_t)(sequence & 0xFFU);
    uart_tx_buffer[5] = (uint8_t)((sequence >> 8) & 0xFFU);
    uart_tx_buffer[6] = (uint8_t)((sequence >> 16) & 0xFFU);
    uart_tx_buffer[7] = (uint8_t)((sequence >> 24) & 0xFFU);
    uart_tx_buffer[8] = (uint8_t)(AUDIO_BLOCK_BYTES & 0xFFU);
    uart_tx_buffer[9] = (uint8_t)((AUDIO_BLOCK_BYTES >> 8) & 0xFFU);

    for (uint32_t i = 0U; i < AUDIO_BLOCK_SAMPLE_COUNT; i++) {
        int32_t sample = dfsdm_dma[offset + i] >> 8;
        int16_t pcm_sample = (int16_t)sample;
        uint32_t sample_offset = UART_HEADER_SIZE + 2U * i;
        uart_tx_buffer[sample_offset] = (uint8_t)((uint16_t)pcm_sample & 0xFFU);
        uart_tx_buffer[sample_offset + 1U] = (uint8_t)(((uint16_t)pcm_sample >> 8) & 0xFFU);
    }

    uint32_t crc;
    if (!calculate_crc32(&uart_tx_buffer[2], (uint16_t)(8U + AUDIO_BLOCK_BYTES), &crc)) {
        audio_uart_errors++;
        audio_fault = true;
        return;
    }

    uint16_t crc_offset = (uint16_t)(UART_HEADER_SIZE + AUDIO_BLOCK_BYTES);
    uart_tx_buffer[crc_offset] = (uint8_t)(crc & 0xFFU);
    uart_tx_buffer[crc_offset + 1U] = (uint8_t)((crc >> 8) & 0xFFU);
    uart_tx_buffer[crc_offset + 2U] = (uint8_t)((crc >> 16) & 0xFFU);
    uart_tx_buffer[crc_offset + 3U] = (uint8_t)(crc >> 24);

    uart_tx_busy = true;
    __DMB();
    if (HAL_UART_Transmit_DMA(&huart3, uart_tx_buffer, (uint16_t)(crc_offset + UART_CRC_SIZE)) != HAL_OK) {
        uart_tx_busy = false;
        audio_uart_errors++;
        audio_fault = true;
        return;
    }

    audio_uart_blocks++;
}

static void start_recording(void) {
    if (recording) {
        return;
    }

    ready_blocks = 0U;
    audio_packet_sequence = 0U;
    recording = true;
    if (HAL_DFSDM_FilterRegularStart_DMA(&hdfsdm1_filter0, dfsdm_dma, AUDIO_DMA_SAMPLE_COUNT) != HAL_OK) {
        recording = false;
        audio_fault = true;
        return;
    }
}

static void stop_recording(void) {
    if (!recording) {
        return;
    }

    recording = false;
    if (HAL_DFSDM_FilterRegularStop_DMA(&hdfsdm1_filter0) != HAL_OK) {
        audio_fault = true;
    }

    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    ready_blocks = 0U;
    __set_PRIMASK(primask);
}

void app_run(void) {
    indicator_update();

    if (HAL_UART_Receive_IT(&huart3, &uart_rx_byte, 1U) != HAL_OK) {
        Error_Handler();
    }

    while (true) {
        indicator_update();

        if (audio_fault) {
            Error_Handler();
        }

        record_command_t command;
        while (take_command(&command)) {
            if (command == RECORD_COMMAND_START) {
                start_recording();
            } else {
                stop_recording();
            }
        }

        uint8_t block;
        uint32_t sequence;
        if (recording && take_ready_block(&block, &sequence)) {
            transmit_block(block, sequence);
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

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *uart) {
    if (uart == &huart3) {
        parse_uart_byte(uart_rx_byte);
        if (HAL_UART_Receive_IT(&huart3, &uart_rx_byte, 1U) != HAL_OK) {
            audio_uart_errors++;
            audio_fault = true;
        }
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
