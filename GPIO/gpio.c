#include "gpio.h"
#include "stm32f4xx.h"

#define LED1_Port  GPIOE
#define LED1_Pin   13

#define LED2_Port  GPIOE
#define LED2_Pin   14

#define LED3_Port  GPIOE
#define LED3_Pin   15

#define S1_Port  GPIOE
#define S1_Pin   10

#define S2_Port  GPIOE
#define S2_Pin   11

#define S3_Port  GPIOE
#define S3_Pin   12

GPIO_TypeDef* LED_ports[LED_NUM] = {[LED_1] = LED1_Port, [LED_2] = LED2_Port, [LED_3] = LED3_Port};
uint8_t LED_pins[LED_NUM]        = {[LED_1] = LED1_Pin, [LED_2] = LED2_Pin, [LED_3] = LED3_Pin};

GPIO_TypeDef * SW_ports[KEY_NUM] = {[KEY_1] = S1_Port, [KEY_2] = S2_Port, [KEY_3] = S3_Port};
uint8_t SW_pins[KEY_NUM] = {[KEY_1] = S1_Pin, [KEY_2] = S2_Pin, [KEY_3] = S3_Pin};

static void EXTI_init(void) {
  /* Enable clocking for SYSCFG for configuring GPIO as EXTI */
  SET_BIT(RCC->APB2ENR, RCC_APB2ENR_SYSCFGEN);

  /* Enable PE10 as external interrupts */
  SET_BIT(SYSCFG->EXTICR[2], SYSCFG_EXTICR3_EXTI10_PE);
  /* Enable PE11 as external interrupts */
  SET_BIT(SYSCFG->EXTICR[2], SYSCFG_EXTICR3_EXTI11_PE);
  /* Enable PE12 as external interrupts */
  SET_BIT(SYSCFG->EXTICR[3], SYSCFG_EXTICR4_EXTI12_PE);

  /* Set interrupt mask and */
  SET_BIT(EXTI->IMR, EXTI_IMR_MR10);
  SET_BIT(EXTI->IMR, EXTI_IMR_MR11);
  SET_BIT(EXTI->IMR, EXTI_IMR_MR12);

  /* Set falling trigger detector */
  SET_BIT(EXTI->FTSR, EXTI_FTSR_TR10);
  SET_BIT(EXTI->FTSR, EXTI_FTSR_TR11);
  SET_BIT(EXTI->FTSR, EXTI_FTSR_TR12);

  /* Drop pending flag */
  SET_BIT(EXTI->PR, EXTI_PR_PR10);
  SET_BIT(EXTI->PR, EXTI_PR_PR11);
  SET_BIT(EXTI->PR, EXTI_PR_PR12);
  
  /* Set priority */
  NVIC_SetPriority(EXTI15_10_IRQn, 14);

  /* Enable interrupt in NVIC */
  NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void GPIO_init(void) {
  /* Init user buttons on board */

    /* Enable GPIOE clock */
  SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOEEN);
  /* Init SW1 (PE10) to input, no pull-up/pull-down */
  CLEAR_BIT(GPIOE->MODER, GPIO_MODER_MODE10_0 | GPIO_MODER_MODE10_1);
  /* Init SW2 (PE11) to input, no pull-up/pull-down */
  CLEAR_BIT(GPIOE->MODER, GPIO_MODER_MODE11_0 | GPIO_MODER_MODE11_1);
  /* Init SW3 (PE12) to input, no pull-up/pull-down */
  CLEAR_BIT(GPIOE->MODER, GPIO_MODER_MODE12_0 | GPIO_MODER_MODE12_1);

  #if(USING_EXTI) == 0
  EXTI_init();
  #endif

  /* Init leds */
  /* Set mode to output for LED1 (PE13) */
  SET_BIT(GPIOE->MODER, GPIO_MODER_MODER13_0);
  SET_BIT(GPIOE->BSRR, GPIO_BSRR_BS13);
  /* Set mode to output for LED1 (PE14) */
  SET_BIT(GPIOE->MODER, GPIO_MODER_MODER14_0);
  SET_BIT(GPIOE->BSRR, GPIO_BSRR_BS14);
  /* Set mode to output for LED1 (PE15) */
  SET_BIT(GPIOE->MODER, GPIO_MODER_MODER15_0);
  SET_BIT(GPIOE->BSRR, GPIO_BSRR_BS15);
}

void led_off(uint8_t led) {
  SET_BIT(LED_ports[led]->BSRR, 1 << LED_pins[led]);
}

void led_on(uint8_t led) {
  SET_BIT(LED_ports[led]->BSRR, 1 << (LED_pins[led] + 16));
}

void led_toggle(LedName led) 
{
  if (READ_BIT(LED_ports[led]->ODR, 1 << LED_pins[led]) == (1 << LED_pins[led])) 
  {
    SET_BIT(LED_ports[led]->BSRR, 1 << (LED_pins[led] + 16));
  } else 
  {
    SET_BIT(LED_ports[led]->BSRR, 1 << LED_pins[led]);
  }
}

uint8_t key_status(KeyName key) {
  if (READ_BIT(SW_ports[key]->IDR, 1 << SW_pins[key]) == (1 << SW_pins[key])) 
  {
    return 1;
  } 
  else 
  {
    return 0;
  }
}