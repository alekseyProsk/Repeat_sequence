#include "stm32f4xx.h"
#include "rng.h"

void rng_init(void) {
  /* Enable RNG clock */
  SET_BIT(RCC->AHB2ENR, RCC_AHB2ENR_RNGEN);
  /* Run RNG */
  SET_BIT(RNG->CR, RNG_CR_RNGEN);
}

uint32_t rng_random(void) {
  while (RESET == READ_BIT(RNG->SR, RNG_SR_DRDY));
  
  return RNG->DR;
}