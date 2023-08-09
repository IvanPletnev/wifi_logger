/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
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
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "esp8266.h"
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticTask_t osStaticThreadDef_t;
typedef StaticQueue_t osStaticMessageQDef_t;
typedef StaticTimer_t osStaticTimerDef_t;
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart2_tx;

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for parcer */
osThreadId_t parcerHandle;
uint32_t parcerTaskBuffer[ 512 ];
osStaticThreadDef_t parcerTaskControlBlock;
const osThreadAttr_t parcer_attributes = {
  .name = "parcer",
  .cb_mem = &parcerTaskControlBlock,
  .cb_size = sizeof(parcerTaskControlBlock),
  .stack_mem = &parcerTaskBuffer[0],
  .stack_size = sizeof(parcerTaskBuffer),
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for uartTx */
osThreadId_t uartTxHandle;
uint32_t uartTxBuffer[ 256 ];
osStaticThreadDef_t uartTxControlBlock;
const osThreadAttr_t uartTx_attributes = {
  .name = "uartTx",
  .cb_mem = &uartTxControlBlock,
  .cb_size = sizeof(uartTxControlBlock),
  .stack_mem = &uartTxBuffer[0],
  .stack_size = sizeof(uartTxBuffer),
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for uartRxQueue */
osMessageQueueId_t uartRxQueueHandle;
uint8_t uartRxQBuffer[ 2 * sizeof( uartRxData_t ) ];
osStaticMessageQDef_t uartRxQControlBlock;
const osMessageQueueAttr_t uartRxQueue_attributes = {
  .name = "uartRxQueue",
  .cb_mem = &uartRxQControlBlock,
  .cb_size = sizeof(uartRxQControlBlock),
  .mq_mem = &uartRxQBuffer,
  .mq_size = sizeof(uartRxQBuffer)
};
/* Definitions for uartTxQueue */
osMessageQueueId_t uartTxQueueHandle;
uint8_t uartTxQueueBuffer[ 2 * sizeof( uartTxData_t ) ];
osStaticMessageQDef_t uartTxQueueControlBlock;
const osMessageQueueAttr_t uartTxQueue_attributes = {
  .name = "uartTxQueue",
  .cb_mem = &uartTxQueueControlBlock,
  .cb_size = sizeof(uartTxQueueControlBlock),
  .mq_mem = &uartTxQueueBuffer,
  .mq_size = sizeof(uartTxQueueBuffer)
};
/* Definitions for replyQ */
osMessageQueueId_t replyQHandle;
uint8_t replyQBuffer[ 3 * sizeof( uint16_t ) ];
osStaticMessageQDef_t replyQControlBlock;
const osMessageQueueAttr_t replyQ_attributes = {
  .name = "replyQ",
  .cb_mem = &replyQControlBlock,
  .cb_size = sizeof(replyQControlBlock),
  .mq_mem = &replyQBuffer,
  .mq_size = sizeof(replyQBuffer)
};
/* Definitions for espTimer */
osTimerId_t espTimerHandle;
osStaticTimerDef_t espTimerControlBlock;
const osTimerAttr_t espTimer_attributes = {
  .name = "espTimer",
  .cb_mem = &espTimerControlBlock,
  .cb_size = sizeof(espTimerControlBlock),
};
/* USER CODE BEGIN PV */
uint8_t statusFlag = 0;
uint8_t initFlag = 0;
uint16_t espSecCounter = 0;
uint8_t espPingFlag = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART1_UART_Init(void);
void StartDefaultTask(void *argument);
extern void parcerTask(void *argument);
extern void uartTxTask(void *argument);
void espTimerCallback(void *argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

	HAL_GPIO_WritePin(TXRX_GPIO_Port, TXRX_Pin, SET);

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
	/* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
	/* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* Create the timer(s) */
  /* creation of espTimer */
  espTimerHandle = osTimerNew(espTimerCallback, osTimerOnce, NULL, &espTimer_attributes);

  /* USER CODE BEGIN RTOS_TIMERS */
	/* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of uartRxQueue */
  uartRxQueueHandle = osMessageQueueNew (2, sizeof(uartRxData_t), &uartRxQueue_attributes);

  /* creation of uartTxQueue */
  uartTxQueueHandle = osMessageQueueNew (2, sizeof(uartTxData_t), &uartTxQueue_attributes);

  /* creation of replyQ */
  replyQHandle = osMessageQueueNew (3, sizeof(uint16_t), &replyQ_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
	/* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of parcer */
  parcerHandle = osThreadNew(parcerTask, NULL, &parcer_attributes);

  /* creation of uartTx */
  uartTxHandle = osThreadNew(uartTxTask, NULL, &uartTx_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
	/* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
	/* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 460800;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);
  /* DMA1_Channel7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel7_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel7_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(TXRX_GPIO_Port, TXRX_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : TXRX_Pin */
  GPIO_InitStruct.Pin = TXRX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(TXRX_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
int _write(int file, char *ptr, int len) {
	(void) file;
	HAL_UART_Transmit(&huart1, (const uint8_t*) ptr, len, 100);
	return len;
}
/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN 5 */

	osDelay(2000);
	MX_USART2_UART_Init();
	HAL_UARTEx_ReceiveToIdle_DMA(&huart2, espHandler.uartRxBuf.buffer,
			UART2RX_BUFFER_SIZE);
	memset ((uint8_t*)espHandler.uartRxBuf.buffer, 0, UART2RX_BUFFER_SIZE);
	initFlag = 1;
	/* Infinite loop */
	for (;;) {
		if (initFlag) {
			initFlag = 0;
			esp8266Init();
		}
		if (espPingFlag){
			espPingFlag = 0;
	    	uartTxDataQueueSendFromIsr((uint8_t*)"ping", 4);
	    	osTimerStart(espTimerHandle, 5000);
		}
		osDelay(1);
	}
  /* USER CODE END 5 */
}

/* espTimerCallback function */
void espTimerCallback(void *argument)
{
  /* USER CODE BEGIN espTimerCallback */
	initFlag = 1;
  /* USER CODE END espTimerCallback */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM4 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */
	static uint16_t espCounter = 0;

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM4) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */
  if (htim->Instance == TIM4) {
    if (espCounter++ > 999) {
    	espCounter = 0;
    	espSecCounter++;
    }
    if (espSecCounter > 9) {
    	espSecCounter = 0;
    	espPingFlag = 1;
    }
  }
  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
