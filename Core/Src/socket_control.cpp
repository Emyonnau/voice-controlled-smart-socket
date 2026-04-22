#include "socket_control.h"
#include "app_config.h"

#include "main.h"
#include "tim.h"
#include "gpio.h"

static volatile uint8_t socket_state = 0;

static void delay350Microseconds(uint32_t multiplier);
static void transmit(uint32_t numberHighPulses, uint32_t numberLowPulses);
static void sendSequence(uint16_t bitSequence);

void socket_control_init(void)
{
    socket_state = 0;
}

void toggle_socket(uint8_t state)
{
    if (state == 1)
    {
        sendSequence(SOCKET_ON_CODE);
        socket_state = 1;
    }
    else
    {
        sendSequence(SOCKET_OFF_CODE);
        socket_state = 0;
    }
}

uint8_t socket_control_get_state(void)
{
    return socket_state;
}

static void delay350Microseconds(uint32_t multiplier)
{
    uint32_t delay = 330U * multiplier;

    __HAL_TIM_SET_COUNTER(&htim4, 0);

    while (__HAL_TIM_GET_COUNTER(&htim4) <= delay)
    {
    }
}

static void transmit(uint32_t numberHighPulses, uint32_t numberLowPulses)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
    delay350Microseconds(numberHighPulses);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
    delay350Microseconds(numberLowPulses);
}

static void sendSequence(uint16_t bitSequence)
{
    for (int x = 0; x < 10; x++)
    {
        const int length = 12;

        for (int i = length - 1; i >= 0; i--)
        {
            transmit(1, 3);

            if (bitSequence & (1U << i))
            {
                transmit(1, 3);
            }
            else
            {
                transmit(3, 1);
            }
        }

        transmit(1, 31);
    }
}