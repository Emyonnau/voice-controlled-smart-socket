#include "main.h"
#include "app.h"
#include "dma.h"
#include "eth.h"
#include "i2s.h"
#include "tim.h"
#include "usart.h"
#include "usb_otg.h"
#include "gpio.h"

void SystemClock_Config(void);

int main(void)
{
  HAL_Init();

  SystemClock_Config();

  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ETH_Init();
  MX_I2S2_Init();
  MX_TIM4_Init();
  MX_USART3_UART_Init();
  MX_USB_OTG_FS_PCD_Init();

  app_init();

  while (1)
  {
    app_process();
  }
}

void SystemClock_Config(void)
{
  /* 
   * Оставь здесь свою cube-generated реализацию, если она у тебя уже есть.
   * Если пока проекта не собираешь, можно оставить пустую заглушку.
   */
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  (void)file;
  (void)line;
}
#endif