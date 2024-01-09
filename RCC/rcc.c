#include "stm32f4xx.h"

void system_clock_168m_25m_hse(void)
{
    RCC->CR |= (1<<16); //HSE_ON

for(int StartCounter=0; ;StartCounter++)
{
  //wait HSE
  if((RCC->CR & (1<<17))){
    break;
  } 
  if(StartCounter > 1000){
    RCC->CR &= ~(1<<16); //HSE_OFF
  }
}

RCC->CR &= ~(1<<24); //PLL_OFF
RCC->PLLCFGR &= ~((1<<5) | (1<<4) | (1<<3) | (1<<2) | (1<<1) | (1<<0)); //Clear M
RCC->PLLCFGR |= (1<<0) | (1<<2); //M=5
RCC->PLLCFGR &= ~((1<<14) | (1<<13) | (1<<12) | (1<<11) | (1<<10) | (1<<9) | (1<<8) | (1<<7) | (1<<6)); //Clear N
RCC->PLLCFGR |= (1<<12); //N=64
RCC->PLLCFGR &= ~((1<<16) | (1<<17)); //P=2
RCC->PLLCFGR |= (1<<22); //PLL_sel_HSE
RCC->CR|= (1<<24); //PLL_ON

for(int StartCounter=0; ;StartCounter++)
{
  //wait PLL
  if((RCC->CR & (1<<25))){
    break;
  } 
  if(StartCounter > 1000){
    RCC->CR &= ~(1<<16); //HSE_OFF
    RCC->CR &= ~(1<<24); //PLL_OFF
  }
}

FLASH->ACR |= (1<<2) | (1<<1);
//FLASH->ACR |= (1<<10) | (1<<9) | (1<<8);

RCC->CFGR &= ~(1<<7); // 160MHz AHB_PRESC_1
RCC->CFGR &= ~(1<<11);
RCC->CFGR |= (1<<12) | (1<<10); // 40 MHz APB1_PRESC_4
RCC->CFGR &= ~(1<<14) | (1<<13);
RCC->CFGR |= (1<<15); //80 MHz APB2_PRESC_2

RCC->CFGR &= ~(1<<0);
RCC->CFGR |= (1<<1); //PLL_sel_clock

//wait set PLL
while((RCC->CFGR & (1<<3)) == 0){}
RCC->CR &= ~(1<<0); //HSI_OFF
SystemCoreClockUpdate();
} 