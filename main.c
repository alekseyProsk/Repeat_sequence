/*********************************************************************
*                    SEGGER Microcontroller GmbH                     *
*                        The Embedded Experts                        *
**********************************************************************



File    : main.c
Purpose : Generic application start
*/
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stm32f4xx.h"
#include "stm32f407xx.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"
#include "stdbool.h"
#include "timers.h"


#include "usart_dbg.h"
#include "rng.h"
#include "rcc.h"
#include "gpio.h"


#define STATIC_TASK_STACK_SIZE (512)
//Difficulty levels [ms]
#define Im_too_young_to_die   1000
#define Hey_not_too_rough     500
#define Hurt_me_plenty        250
#define Ultra_Violence        100
#define Nightmare             50
//Time startup after choose difficulty level
#define  Time_startup         2000;
//Size array sequences [pcs]
#define  Size_array           5
//Time wait user press botton [ms]
#define Time_wait             3000

#define SYNC_QUEUE_LEN (5)
xQueueHandle sync_queue;



//timer handler
static xTimerHandle Timer;
static xTimerHandle TimerGame;
//Tasks hendlers
static xTaskHandle UsartHendle;
static xTaskHandle ControlTaskHandle;
static xTaskHandle MainGameHandle;

static StaticTask_t xTaskControlBlock;
static StackType_t xTaskStack[STATIC_TASK_STACK_SIZE];

//st
static SemaphoreHandle_t  mutexBuffer;


void TaskUsart(void* pvParameter);
void TaskControl(void* pvParameter);
void flashRound(uint8_t round);
LedName checkLeds(uint8_t keyNym);
void vTIMER(xTimerHandle Timer);

uint8_t array[Size_array] = {};
uint8_t UserAnswers[Size_array] = {};
uint16_t difficultyLevel = 0;

static uint8_t countRoundAnswers = 0;
static uint8_t needAnswers = 1;
static uint8_t round = 1;

//bool next = true;
static bool gameOver = false;
static bool isWin = false;
static bool nextRound = true;

/* configSUPPORT_STATIC_ALLOCATION is set to 1, so the application must provide an
implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
used by the Idle task. */
 /*GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer,
                                     StackType_t **ppxTimerTaskStackBuffer,
                                     uint32_t *pulTimerTaskStackSize )
{
   /* Если буферы, предоставленные для Timer task, декларированы внутри этой
      функции, то они должны быть с атрибутом static – иначе буферы будут
      уничтожены при выходе из этой функции. */
   static StaticTask_t xTimerTaskTCB;
   static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];
 
   /* Передача наружу указателя на структуру StaticTask_t, в которой будет
      сохраняться состояние задачи Timer service. */
   *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
 
   /* Передача наружу массива, который будет использоваться как стек
      задачи Timer service. */
   *ppxTimerTaskStackBuffer = uxTimerTaskStack;
 
   /* Передача наружу размера массива, на который указывает *ppxTimerTaskStackBuffer.
      Обратите внимание, что поскольку массив должен быть типа StackType_t, то
      configTIMER_TASK_STACK_DEPTH указывается в словах, а не в байтах. */
   *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];
  
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/*-----------------------------------------------------------*/


void Rand_Array(void)
{
  for(int i=0; i < Size_array; i++)
  {
    array[i] = ((float)rng_random()/RNG_RANDOM_MAX)*3+1;
  }
}


void TaskUsart(void* pvParameter) // task for all prints Uart
{
  uint32_t recieveSymbol;
  bool newGame = true;
  for(;;)
  {
    if(gameOver == true)
    {
      xTimerStop(TimerGame, 0);
      difficultyLevel = 0;
      USART_TX("\n\n\rYOU DIED\n\n\r");
      newGame = true;
      gameOver = false;
    }
    if(newGame == true)
    {
      USART_TX("Chose difficulty level:\r\n");
      USART_TX("1 - Im too young to die\r\n"
      "2 - Hey not too rough\n\r3 - Hurt me plenty\r\n"
      "4 - Ultra_Violence\r\n5 - Nightmare");
      newGame = false;
    }
    if(difficultyLevel != 0)
    {
      xTimerStop(TimerGame,0);
      USART_TX("\n\rGame started after 3 seconds\r\n");
      vTaskDelay(3000);
      Rand_Array();
      nextRound = true;
      vTaskResume(MainGameHandle);
      vTaskResume(ControlTaskHandle);
      vTaskSuspend(UsartHendle);
    }
    if(isWin == true)
    {
      USART_TX("\n\n\rCONGRATULATIONS !!!\n\n\r");
      xTimerStop(TimerGame, 0);
      for(int i = 0; i < 3; i++)
      {
        led_on(LED_1);
        led_on(LED_2);
        led_on(LED_3);
        vTaskDelay(500);
        led_off(LED_1);
        led_off(LED_2);
        led_off(LED_3);
        vTaskDelay(500);
      }
      isWin = false;
      difficultyLevel = 0;
      newGame = true;
    }
  }
  vTaskDelete(UsartHendle);
}


void MainGame(void* pvParameter)// Task for control rules the Game
{
   
  for(;;)
  { 
    if(nextRound == true )
      {
        NVIC_DisableIRQ(EXTI15_10_IRQn);
        xTimerStop(TimerGame, 0);
        flashRound(round);
        nextRound = false;
        NVIC_EnableIRQ(EXTI15_10_IRQn);
        xTimerStart(TimerGame, 0);
      }
    if(xSemaphoreTake(mutexBuffer, portMAX_DELAY) == pdTRUE)
    {
      
      if(round == countRoundAnswers)
      { 
          xTimerStop(TimerGame, 0);
          nextRound = true;
          memset(UserAnswers, 0, sizeof(UserAnswers));
          countRoundAnswers = 0;
          round++;
          vTaskDelay(500);
          xTimerStart(TimerGame, 0);
      }
      
      if(gameOver == true)
      {   
          xTimerStop(TimerGame, 0);
          countRoundAnswers = 0; 
          round = 1;
          memset(UserAnswers, 0, sizeof(UserAnswers));
          vTaskResume(UsartHendle);
          vTaskSuspend(ControlTaskHandle);
          vTaskSuspend(MainGameHandle);
      }
      if(round == (Size_array + 1))
      {
          xTimerStop(TimerGame, 0);
          countRoundAnswers = 0; 
          isWin = true;
          round = 1;
          memset(UserAnswers, 0, sizeof(UserAnswers));
          vTaskResume(UsartHendle);
          vTaskSuspend(ControlTaskHandle);
          vTaskSuspend(MainGameHandle);
      }
      
      xSemaphoreGive(mutexBuffer);
    }
   
  }
}

void TaskControl(void* pvParameter) // Task for control keyPress
{
  uint8_t keyNum;
 
  for(;;)
  { 
  
     xQueueReceive(sync_queue, &keyNum, portMAX_DELAY);
     xTimerStart(TimerGame, 0);
    if(xSemaphoreTake(mutexBuffer, portMAX_DELAY) == pdTRUE)
    {
      if((key_status(keyNum)) == 1)
      {
        switch (keyNum)
        {
        case KEY_1:
        {
          UserAnswers[countRoundAnswers] = 1;
          countRoundAnswers++;
          break;
        }
        case KEY_2:
        {
          UserAnswers[countRoundAnswers] = 2;
          countRoundAnswers++;
          break;
        }
        case KEY_3:
        {
          UserAnswers[countRoundAnswers] = 3;
          countRoundAnswers++;
          break;
        }
        }
      }

      xTimerStart(TimerGame, 0);
      while((key_status(keyNum)) == 0){};
      //NVIC_EnableIRQ(EXTI15_10_IRQn);
      if(memcmp(array, UserAnswers, countRoundAnswers) != 0)
      {
          gameOver = true;
          xTimerStop(TimerGame, 0);
      }
      xTimerStart(TimerGame, 0);
      xSemaphoreGive(mutexBuffer);
      //keyNum = 0;
    }
    
  }
}



void flashRound(uint8_t round) // flash all subsequence leds
{
  for(int i = 0; i < round; i++)
  {
    
    led_on(checkLeds(array[i]));
    vTaskDelay(difficultyLevel);
    led_off(checkLeds(array[i]));
    vTaskDelay(difficultyLevel);
  }
  led_off(LED_1);
  led_off(LED_2);
  led_off(LED_3);
}
LedName checkLeds(uint8_t keyNym)
{
  switch(keyNym)
  {
    case 1:
      return LED_1;
    break;
    case 2:
      return LED_2;
    break;
    case 3:
      return LED_3;
    break;
  }
}


void TimerAFK(xTimerHandle pvTimer)// timer for Exit for end of game if afk time > 3 sec
{
  if(isWin != true || gameOver != true)
  {
    gameOver = true;
    USART_TX("\n\n\rTime Out\n\n\r");
    xTimerStop(TimerGame, 0);
  }
}
/*********************************************************************
*
*       main()
*
*  Function description
*   Application entry point.
*/

int main(void) 
{
  system_clock_168m_25m_hse();
  usart1_init();
  GPIO_init();
  rng_init();
  
  Timer = xTimerCreate("TIM1", pdMS_TO_TICKS(300), pdFALSE, (void *)0, vTIMER);
  TimerGame = xTimerCreate("TimerAFK", pdMS_TO_TICKS(3000), pdFALSE, (void *)0, TimerAFK);
  xTimerStop(TimerGame, 0);

  mutexBuffer = xSemaphoreCreateMutex();
  sync_queue = xQueueCreate(SYNC_QUEUE_LEN, sizeof(KeyName));
  
  UsartHendle = xTaskCreateStatic(TaskUsart, "TaskUsart", STATIC_TASK_STACK_SIZE, NULL, 3, xTaskStack, &xTaskControlBlock);
  xTaskCreate(TaskControl, "TaskControl", configMINIMAL_STACK_SIZE, NULL, 2, &ControlTaskHandle);
  vTaskSuspend(ControlTaskHandle);
  xTaskCreate(MainGame, "mainGame", configMINIMAL_STACK_SIZE, NULL, 2, &MainGameHandle);
  vTaskSuspend(MainGameHandle);
  
  
  vTaskStartScheduler();
  return  0;
}

//Timer for Exti interrupts
void vTIMER(xTimerHandle Timer)
{
    xTimerStop(Timer, 0);
    //NVIC_DisableIRQ(EXTI15_10_IRQn);
    NVIC_ClearPendingIRQ(EXTI15_10_IRQn);
    NVIC_EnableIRQ(EXTI15_10_IRQn);
    //xTimerStart(Timer, 0);
}

void USART1_IRQHandler(void)
{ 
  
  if((USART1->SR & USART_SR_RXNE) )
  {
    NVIC_DisableIRQ(USART1_IRQn);
    uint16_t symbol = (uint16_t)USART1->DR & ((uint16_t)0x01FF);
    switch(symbol)
    {
    case '1': 
        difficultyLevel = Im_too_young_to_die;
        break;
    case '2':
        difficultyLevel = Hey_not_too_rough;
        break; 
    case '3':
        difficultyLevel = Hurt_me_plenty;
        break;
    case '4':
        difficultyLevel = Ultra_Violence;
        break;
    case '5':
        difficultyLevel = Nightmare;
    default:
        break;
    }
    
    NVIC_EnableIRQ(USART1_IRQn);
  }
}

void EXTI15_10_IRQHandler(void)
{ 
  
 
  if(difficultyLevel != 0)
  {
    uint32_t exti_line;
    KeyName key;
    
    if(READ_BIT(EXTI->PR, EXTI_PR_PR10) == EXTI_PR_PR10)
    {
      key         = KEY_1;
      led_on(LED_1);
      while((GPIOE->IDR & GPIO_IDR_ID10) == 0){};
      led_off(LED_1);
      exti_line   = EXTI_PR_PR10;
      NVIC_DisableIRQ(EXTI15_10_IRQn);
     xTimerStartFromISR(Timer, 0);


    } 
    else if(READ_BIT(EXTI->PR, EXTI_PR_PR11) == EXTI_PR_PR11)
    {
      key         = KEY_2;
      led_on(LED_2);
      while((GPIOE->IDR & GPIO_IDR_ID11) == 0){}; 
      led_off(LED_2);
      exti_line   = EXTI_PR_PR11;
      NVIC_DisableIRQ(EXTI15_10_IRQn);
      xTimerStartFromISR(Timer, 0);
    } 
    else if(READ_BIT(EXTI->PR, EXTI_PR_PR12) == EXTI_PR_PR12)
    {
      key         = KEY_3;
      led_on(LED_3);
      while((GPIOE->IDR & GPIO_IDR_ID12) == 0){}; 
      led_off(LED_3);
      exti_line   = EXTI_PR_PR12;
      NVIC_DisableIRQ(EXTI15_10_IRQn);
      xTimerStartFromISR(Timer, 0);
    } 
 
    SET_BIT(EXTI->PR, exti_line);
    BaseType_t taskWoken;
    xQueueSendFromISR(sync_queue, &key, &taskWoken);
    portYIELD_FROM_ISR(taskWoken);
  }

}