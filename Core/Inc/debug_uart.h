#ifndef DEBUG_UART_H
#define DEBUG_UART_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void UART_SendString(char *string);

void ControlLEDs(uint8_t numLEDs);
uint8_t mapVolumeToLEDs(uint32_t averageVolume);
void DisplayVolume(int32_t volumeValue);

int __io_putchar(int ch);
int _write(int file, char *ptr, int len);

#ifdef __cplusplus
}
#endif

#endif