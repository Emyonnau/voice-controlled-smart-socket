#include "debug_uart.h"

#include "main.h"
#include "gpio.h"
#include "usart.h"

#include <string.h>

void ControlLEDs(uint8_t numLEDs)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_3, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_5, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_2, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_1, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_0, GPIO_PIN_RESET);

    if (numLEDs >= 1) HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
    if (numLEDs >= 2) HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);
    if (numLEDs >= 3) HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET);
    if (numLEDs >= 4) HAL_GPIO_WritePin(GPIOF, GPIO_PIN_3, GPIO_PIN_SET);
    if (numLEDs >= 5) HAL_GPIO_WritePin(GPIOF, GPIO_PIN_5, GPIO_PIN_SET);
    if (numLEDs >= 6) HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_SET);
    if (numLEDs >= 7) HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_SET);
    if (numLEDs >= 8) HAL_GPIO_WritePin(GPIOF, GPIO_PIN_2, GPIO_PIN_SET);
    if (numLEDs >= 9) HAL_GPIO_WritePin(GPIOF, GPIO_PIN_1, GPIO_PIN_SET);
    if (numLEDs >= 10) HAL_GPIO_WritePin(GPIOF, GPIO_PIN_0, GPIO_PIN_SET);
}

void UART_SendString(char *string)
{
    HAL_UART_Transmit(&huart3, (uint8_t *)string, strlen(string), HAL_MAX_DELAY);
}

uint8_t mapVolumeToLEDs(uint32_t averageVolume)
{
    if (averageVolume <= 5000U)
    {
        return 1;
    }
    else if (averageVolume <= 10000U)
    {
        return 2;
    }
    else if (averageVolume <= 20000U)
    {
        return 3;
    }
    else if (averageVolume <= 40000U)
    {
        return 4;
    }
    else if (averageVolume <= 60000U)
    {
        return 5;
    }
    else if (averageVolume <= 80000U)
    {
        return 6;
    }
    else if (averageVolume <= 100000U)
    {
        return 7;
    }
    else
    {
        return 8;
    }
}

void DisplayVolume(int32_t volumeValue)
{
    uint32_t absVolume;

    if (volumeValue < 0)
    {
        absVolume = (uint32_t)(-volumeValue);
    }
    else
    {
        absVolume = (uint32_t)volumeValue;
    }

    ControlLEDs(mapVolumeToLEDs(absVolume));
}

int __io_putchar(int ch)
{
    uint8_t c[1];
    c[0] = (uint8_t)(ch & 0x00FF);
    HAL_UART_Transmit(&huart3, c, 1, 10);
    return ch;
}

int _write(int file, char *ptr, int len)
{
    (void)file;

    for (int dataIdx = 0; dataIdx < len; dataIdx++)
    {
        __io_putchar(*ptr++);
    }

    return len;
}