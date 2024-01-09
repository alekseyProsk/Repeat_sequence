/*  S1 = PE10
    S2 = PE11
    S3 = PE12
 */

#include "stdint.h"

#define USING_EXTI  0

typedef enum KeyName_t {
  KEY_1,
  KEY_2,
  KEY_3,

  KEY_NUM
} KeyName; 

typedef enum LedName_t {
  LED_1,
  LED_2,
  LED_3,

  LED_NUM
} LedName; 

void GPIO_init(void);

void led_on(LedName led);
void led_off(LedName led);
void led_toggle(LedName led);

uint8_t key_status(KeyName key);