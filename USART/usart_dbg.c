#include "stm32f4xx.h"

void usart1_init(void) {
  RCC->AHB1ENR |= (1<<0); //IO A clock enable

  GPIOA->MODER |= (2<<18); //PA9 ALT
  GPIOA->AFR[1] |= (7<<4); //AF7
  GPIOA->OTYPER &= ~(1<<9); // push pull
  GPIOA->OSPEEDR |= (3<<18); //hihg speed
  GPIOA->PUPDR &= ~((1<<19) | (1<<18)); //no pull up/down

  GPIOA->MODER |= (2<<20); //PA10 ALT
  GPIOA->AFR[1] |=(7<<8);  //AF7
  GPIOA->PUPDR |= (1<<20); //pull up

  RCC->APB2ENR |= (1<<4); //USART1 clock enable
  USART1->CR1 |= (1<<13); //USART1 enable
  USART1->BRR |= 0x2B6;
  USART1->CR1 &= ~(1<<12); // 8 BIT
  USART1->CR1 &= ~(1<<10); // parity dis
  USART1->CR1 |= (1<<3); // TX enable
  USART1->CR1 |= (1<<2); // RX enable
  USART1->CR2 &= ~((1<<13) | (1<<12)); // STOP 1 BIT
  USART1->CR1 |= USART_CR1_RXNEIE;
  
  NVIC_SetPriority(USART1_IRQn, 14);
  NVIC_EnableIRQ(USART1_IRQn);
 

}


/* retarget the C library printf function to the USART */
int __SEGGER_RTL_X_file_write(__SEGGER_RTL_FILE *__stream, const char *__s, unsigned __len) {
  
  /* Send string over USART1 in pending mode */
  for (; __len != 0; --__len) {
    USART1->DR = * __s++;
    while (RESET == READ_BIT(USART1->SR, USART_SR_TXE));
  } 

  return 0;
}



void USART_TX (char TX[]){
    for(int i = 0; TX[i]; i++){
      while((USART1->SR & (1<<6)) == 0){}
      USART1->DR = TX[i];
    }
}