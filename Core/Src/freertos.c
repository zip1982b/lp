/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#ifdef DEBUG
#include "stdio.h"
#endif
#include "tim.h"
#include "queue.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
enum status // состояние шарового крана
{
    s_open = 1,   // открыт
    s_closed = 2,   // закрыт
    s_closes = 12,    // закрывается
	s_opens = 21 // открывается
};

enum commands // команды управления для шарового крана
{
	no_command = 0,
    open = 1,   // открыть
    close = 2   // закрыть
};



/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for waterValveTask */
osThreadId_t waterValveTaskHandle;
const osThreadAttr_t waterValveTask_attributes = {
  .name = "waterValveTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for myCommandTask */
osThreadId_t myCommandTaskHandle;
const osThreadAttr_t myCommandTask_attributes = {
  .name = "myCommandTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for commandQueue */
osMessageQueueId_t commandQueueHandle;
const osMessageQueueAttr_t commandQueue_attributes = {
  .name = "commandQueue"
};
/* Definitions for WaterAlarmCountingSem */
osSemaphoreId_t WaterAlarmCountingSemHandle;
const osSemaphoreAttr_t WaterAlarmCountingSem_attributes = {
  .name = "WaterAlarmCountingSem"
};
/* Definitions for TimerDoneCountingSem */
osSemaphoreId_t TimerDoneCountingSemHandle;
const osSemaphoreAttr_t TimerDoneCountingSem_attributes = {
  .name = "TimerDoneCountingSem"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartwaterValveTask(void *argument);
void StartmyCommandTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of WaterAlarmCountingSem */
  WaterAlarmCountingSemHandle = osSemaphoreNew(2, 2, &WaterAlarmCountingSem_attributes);

  /* creation of TimerDoneCountingSem */
  TimerDoneCountingSemHandle = osSemaphoreNew(2, 2, &TimerDoneCountingSem_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of commandQueue */
  commandQueueHandle = osMessageQueueNew (5, sizeof(uint8_t), &commandQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of waterValveTask */
  waterValveTaskHandle = osThreadNew(StartwaterValveTask, NULL, &waterValveTask_attributes);

  /* creation of myCommandTask */
  myCommandTaskHandle = osThreadNew(StartmyCommandTask, NULL, &myCommandTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartwaterValveTask */
/**
  * @brief  Function implementing the waterValveTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartwaterValveTask */
void StartwaterValveTask(void *argument)
{
  /* USER CODE BEGIN StartwaterValveTask */
	portBASE_TYPE xStatus;
	enum status st = open;
	enum commands cm = no_command;
	osStatus_t StatusSemAlarm;
	osStatus_t StatusSemT;




	HAL_GPIO_WritePin(CloseValve_GPIO_Port, CloseValve_Pin, GPIO_PIN_RESET);
	st = s_closes; // шаровый кран закрывается
	osDelay(10000);// время для полного закрытия крана
	st = s_closed; // кран закрыт
	HAL_GPIO_WritePin(CloseValve_GPIO_Port, CloseValve_Pin, GPIO_PIN_SET);


  /* Infinite loop */
  for(;;)
  {
	  StatusSemAlarm = osSemaphoreAcquire(WaterAlarmCountingSemHandle, 100);
	  StatusSemT = osSemaphoreAcquire(TimerDoneCountingSemHandle, 100);
	  //osMessageQueueGet(commandQueueHandle, &cm, 0, 200);
	  xStatus = xQueueReceive(commandQueueHandle, &cm, 50);


#ifdef DEBUG
	  if( xStatus == pdPASS )
	  {
	  /* Операция отправки не завершена, потому что очередь была заполнена -
	     это должно означать ошибку, так как в нашем случае очередь никогда
	     не будет содержать больше одного элемента данных! */
	     printf("Received = %d\n", cm);
	  }
#endif


	  switch(st){
	  case s_open:
#ifdef DEBUG
		  printf("water valve is open\n");
#endif
		  if(StatusSemAlarm == osOK || cm == close){
			  HAL_GPIO_WritePin(CloseValve_GPIO_Port, CloseValve_Pin, GPIO_PIN_RESET);
			  st = s_closes;
			  __HAL_TIM_CLEAR_FLAG(&htim1, TIM_SR_UIF);
			  HAL_TIM_Base_Start_IT(&htim1);// timer start (in interrupt handle - HAL_GPIO_WritePin(CloseValve_GPIO_Port, CloseValve_Pin, GPIO_PIN_SET))
#ifdef DEBUG
			  printf("water alarm!\n");
#endif

		  }
		  break;
	  case s_closes:
		  if(StatusSemT == osOK){
			  HAL_GPIO_WritePin(CloseValve_GPIO_Port, CloseValve_Pin, GPIO_PIN_SET);
			  st = s_closed;
		  }
		  printf("water valve is closing...\n");
		  break;
	  case s_closed:
		  printf("water valve is closed\n");
		  if(cm == open){
			  HAL_GPIO_WritePin(OpenValve_GPIO_Port, OpenValve_Pin, GPIO_PIN_RESET);
			  st = s_opens;
			  //timer start
			  __HAL_TIM_CLEAR_FLAG(&htim1, TIM_SR_UIF);
			  HAL_TIM_Base_Start_IT(&htim1);// timer start (in interrupt handle - HAL_GPIO_WritePin(CloseValve_GPIO_Port, CloseValve_Pin, GPIO_PIN_SET))
			  printf("command open valve\n");
		  }
		  break;
	  case s_opens:
		  if(StatusSemT == osOK){
		  	HAL_GPIO_WritePin(OpenValve_GPIO_Port, OpenValve_Pin, GPIO_PIN_SET);
		  	st = s_open;
		  }
		  printf("water valve is opens\n");
		  break;
	  }
	  StatusSemAlarm = 5;
	  StatusSemT = 5;

  }
  /* USER CODE END StartwaterValveTask */
}

/* USER CODE BEGIN Header_StartmyCommandTask */
/**
* @brief Function implementing the myCommandTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartmyCommandTask */
void StartmyCommandTask(void *argument)
{
  /* USER CODE BEGIN StartmyCommandTask */
	enum commands cm = no_command;
	portBASE_TYPE xStatus;
  /* Infinite loop */
  for(;;)
  {
	cm = open;
	osMessageQueuePut(commandQueueHandle, &cm, 0, 200);
    osDelay(20000);
    cm = close;
    xStatus = xQueueSendToBack(commandQueueHandle, &cm, 200);
#ifdef DEBUG
    printf("xStatus = %ld\n", xStatus);
#endif
    //osMessageQueuePut(commandQueueHandle, &cm, 0, 200);
    //osDelay(20000);
    vTaskDelay(20000);


  }
  /* USER CODE END StartmyCommandTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */



/* USER CODE END Application */

