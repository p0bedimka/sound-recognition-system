/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file         stm32f4xx_hal_msp.c
  * @brief        This file provides code for the MSP Initialization
  *               and de-Initialization codes.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */
extern DMA_HandleTypeDef hdma_dfsdm1_flt0;

extern DMA_HandleTypeDef hdma_usart3_tx;

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN Define */

/* USER CODE END Define */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN Macro */

/* USER CODE END Macro */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* External functions --------------------------------------------------------*/
/* USER CODE BEGIN ExternalFunctions */

/* USER CODE END ExternalFunctions */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */
/**
  * Initializes the Global MSP.
  */
void HAL_MspInit(void) {
    /* USER CODE BEGIN MspInit 0 */

    /* USER CODE END MspInit 0 */

    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    /* System interrupt init*/

    /* USER CODE BEGIN MspInit 1 */

    /* USER CODE END MspInit 1 */
}

static uint32_t HAL_RCC_DFSDM1_CLK_ENABLED = 0;

static uint32_t DFSDM1_Init = 0;
/**
  * @brief DFSDM_Filter MSP Initialization
  * This function configures the hardware resources used in this example
  * @param hdfsdm_filter: DFSDM_Filter handle pointer
  * @retval None
  */
void HAL_DFSDM_FilterMspInit(DFSDM_Filter_HandleTypeDef *hdfsdm_filter) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
    if (DFSDM1_Init == 0) {
        /* USER CODE BEGIN DFSDM1_MspInit 0 */

        /* USER CODE END DFSDM1_MspInit 0 */

        /** Initializes the peripherals clock
  */
        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_DFSDM1;
        PeriphClkInitStruct.Dfsdm1ClockSelection = RCC_DFSDM1CLKSOURCE_APB2;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
            Error_Handler();
        }

        /* Peripheral clock enable */
        HAL_RCC_DFSDM1_CLK_ENABLED++;
        if (HAL_RCC_DFSDM1_CLK_ENABLED == 1) {
            __HAL_RCC_DFSDM1_CLK_ENABLE();
        }

        __HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        /**DFSDM1 GPIO Configuration
    PC2     ------> DFSDM1_CKOUT
    PB1     ------> DFSDM1_DATIN0
    */
        GPIO_InitStruct.Pin = GPIO_PIN_2;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
        GPIO_InitStruct.Alternate = GPIO_AF8_DFSDM1;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = GPIO_PIN_1;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF8_DFSDM1;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        /* DFSDM1 interrupt Init */
        HAL_NVIC_SetPriority(DFSDM1_FLT0_IRQn, 1, 0);
        HAL_NVIC_EnableIRQ(DFSDM1_FLT0_IRQn);
        /* USER CODE BEGIN DFSDM1_MspInit 1 */

        /* USER CODE END DFSDM1_MspInit 1 */

        DFSDM1_Init++;
    }

    /* DFSDM1 DMA Init */
    /* DFSDM1_FLT0 Init */
    if (hdfsdm_filter->Instance == DFSDM1_Filter0) {
        hdma_dfsdm1_flt0.Instance = DMA2_Stream0;
        hdma_dfsdm1_flt0.Init.Channel = DMA_CHANNEL_7;
        hdma_dfsdm1_flt0.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_dfsdm1_flt0.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_dfsdm1_flt0.Init.MemInc = DMA_MINC_ENABLE;
        hdma_dfsdm1_flt0.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
        hdma_dfsdm1_flt0.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
        hdma_dfsdm1_flt0.Init.Mode = DMA_CIRCULAR;
        hdma_dfsdm1_flt0.Init.Priority = DMA_PRIORITY_VERY_HIGH;
        hdma_dfsdm1_flt0.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdma_dfsdm1_flt0) != HAL_OK) {
            Error_Handler();
        }

        /* Several peripheral DMA handle pointers point to the same DMA handle.
     Be aware that there is only one stream to perform all the requested DMAs. */
        __HAL_LINKDMA(hdfsdm_filter, hdmaInj, hdma_dfsdm1_flt0);
        __HAL_LINKDMA(hdfsdm_filter, hdmaReg, hdma_dfsdm1_flt0);
    }
}

/**
  * @brief DFSDM_Channel MSP Initialization
  * This function configures the hardware resources used in this example
  * @param hdfsdm_channel: DFSDM_Channel handle pointer
  * @retval None
  */
void HAL_DFSDM_ChannelMspInit(DFSDM_Channel_HandleTypeDef *hdfsdm_channel) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
    if (DFSDM1_Init == 0) {
        /* USER CODE BEGIN DFSDM1_MspInit 0 */

        /* USER CODE END DFSDM1_MspInit 0 */

        /** Initializes the peripherals clock
  */
        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_DFSDM1;
        PeriphClkInitStruct.Dfsdm1ClockSelection = RCC_DFSDM1CLKSOURCE_APB2;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
            Error_Handler();
        }

        /* Peripheral clock enable */
        HAL_RCC_DFSDM1_CLK_ENABLED++;
        if (HAL_RCC_DFSDM1_CLK_ENABLED == 1) {
            __HAL_RCC_DFSDM1_CLK_ENABLE();
        }

        __HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        /**DFSDM1 GPIO Configuration
    PC2     ------> DFSDM1_CKOUT
    PB1     ------> DFSDM1_DATIN0
    */
        GPIO_InitStruct.Pin = GPIO_PIN_2;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF8_DFSDM1;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = GPIO_PIN_1;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF8_DFSDM1;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        /* USER CODE BEGIN DFSDM1_MspInit 1 */

        /* USER CODE END DFSDM1_MspInit 1 */

        DFSDM1_Init++;
    }
}

/**
  * @brief DFSDM_Filter MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param hdfsdm_filter: DFSDM_Filter handle pointer
  * @retval None
  */
void HAL_DFSDM_FilterMspDeInit(DFSDM_Filter_HandleTypeDef *hdfsdm_filter) {
    DFSDM1_Init--;
    if (DFSDM1_Init == 0) {
        /* USER CODE BEGIN DFSDM1_MspDeInit 0 */

        /* USER CODE END DFSDM1_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_DFSDM1_CLK_DISABLE();

        /**DFSDM1 GPIO Configuration
    PC2     ------> DFSDM1_CKOUT
    PB1     ------> DFSDM1_DATIN0
    */
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_2);

        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_1);

        /* DFSDM1 interrupt DeInit */
        HAL_NVIC_DisableIRQ(DFSDM1_FLT0_IRQn);

        /* DFSDM1 DMA DeInit */
        HAL_DMA_DeInit(hdfsdm_filter->hdmaInj);
        HAL_DMA_DeInit(hdfsdm_filter->hdmaReg);
        /* USER CODE BEGIN DFSDM1_MspDeInit 1 */

        /* USER CODE END DFSDM1_MspDeInit 1 */
    }
}

/**
  * @brief DFSDM_Channel MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param hdfsdm_channel: DFSDM_Channel handle pointer
  * @retval None
  */
void HAL_DFSDM_ChannelMspDeInit(DFSDM_Channel_HandleTypeDef *hdfsdm_channel) {
    DFSDM1_Init--;
    if (DFSDM1_Init == 0) {
        /* USER CODE BEGIN DFSDM1_MspDeInit 0 */

        /* USER CODE END DFSDM1_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_DFSDM1_CLK_DISABLE();

        /**DFSDM1 GPIO Configuration
    PC2     ------> DFSDM1_CKOUT
    PB1     ------> DFSDM1_DATIN0
    */
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_2);

        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_1);

        /* USER CODE BEGIN DFSDM1_MspDeInit 1 */

        /* USER CODE END DFSDM1_MspDeInit 1 */
    }
}

/**
  * @brief RTC MSP Initialization
  * This function configures the hardware resources used in this example
  * @param hrtc: RTC handle pointer
  * @retval None
  */
void HAL_RTC_MspInit(RTC_HandleTypeDef *hrtc) {
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
    if (hrtc->Instance == RTC) {
        /* USER CODE BEGIN RTC_MspInit 0 */

        /* USER CODE END RTC_MspInit 0 */

        /** Initializes the peripherals clock
  */
        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
        PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
            Error_Handler();
        }

        /* Peripheral clock enable */
        __HAL_RCC_RTC_ENABLE();
        /* USER CODE BEGIN RTC_MspInit 1 */

        /* USER CODE END RTC_MspInit 1 */
    }
}

/**
  * @brief RTC MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param hrtc: RTC handle pointer
  * @retval None
  */
void HAL_RTC_MspDeInit(RTC_HandleTypeDef *hrtc) {
    if (hrtc->Instance == RTC) {
        /* USER CODE BEGIN RTC_MspDeInit 0 */

        /* USER CODE END RTC_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_RTC_DISABLE();
        /* USER CODE BEGIN RTC_MspDeInit 1 */

        /* USER CODE END RTC_MspDeInit 1 */
    }
}

/**
  * @brief UART MSP Initialization
  * This function configures the hardware resources used in this example
  * @param huart: UART handle pointer
  * @retval None
  */
void HAL_UART_MspInit(UART_HandleTypeDef *huart) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (huart->Instance == USART3) {
        /* USER CODE BEGIN USART3_MspInit 0 */

        /* USER CODE END USART3_MspInit 0 */
        /* Peripheral clock enable */
        __HAL_RCC_USART3_CLK_ENABLE();

        __HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        /**USART3 GPIO Configuration
    PC5     ------> USART3_RX
    PB10     ------> USART3_TX
    */
        GPIO_InitStruct.Pin = GPIO_PIN_5;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        /* USART3 DMA Init */
        /* USART3_TX Init */
        hdma_usart3_tx.Instance = DMA1_Stream3;
        hdma_usart3_tx.Init.Channel = DMA_CHANNEL_4;
        hdma_usart3_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_usart3_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart3_tx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart3_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart3_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart3_tx.Init.Mode = DMA_NORMAL;
        hdma_usart3_tx.Init.Priority = DMA_PRIORITY_LOW;
        hdma_usart3_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdma_usart3_tx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(huart, hdmatx, hdma_usart3_tx);

        /* USART3 interrupt Init */
        HAL_NVIC_SetPriority(USART3_IRQn, 6, 0);
        HAL_NVIC_EnableIRQ(USART3_IRQn);
        /* USER CODE BEGIN USART3_MspInit 1 */

        /* USER CODE END USART3_MspInit 1 */
    }
}

/**
  * @brief UART MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param huart: UART handle pointer
  * @retval None
  */
void HAL_UART_MspDeInit(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART3) {
        /* USER CODE BEGIN USART3_MspDeInit 0 */

        /* USER CODE END USART3_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_USART3_CLK_DISABLE();

        /**USART3 GPIO Configuration
    PC5     ------> USART3_RX
    PB10     ------> USART3_TX
    */
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_5);

        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10);

        /* USART3 DMA DeInit */
        HAL_DMA_DeInit(huart->hdmatx);

        /* USART3 interrupt DeInit */
        HAL_NVIC_DisableIRQ(USART3_IRQn);
        /* USER CODE BEGIN USART3_MspDeInit 1 */

        /* USER CODE END USART3_MspDeInit 1 */
    }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
