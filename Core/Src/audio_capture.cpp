#include "audio_capture.h"
#include "app_config.h"
#include "vad_preroll.h"
#include "main.h"
#include "i2s.h"

#include <stdint.h>

static uint16_t input_buffer[I2S_BUF_SIZE * 2];
static int32_t mergedFrame[I2S_BUF_SIZE / 2];

static void process_audio_data(uint16_t *inputBuffer,
                               int32_t *mergedFrameBuffer,
                               uint16_t buffer_start,
                               uint16_t merged_start,
                               uint16_t length);

void audio_capture_init(void)
{
}

void audio_capture_start(void)
{
    HAL_I2S_Receive_DMA(&hi2s2, &input_buffer[0], I2S_BUF_SIZE);
}

extern "C" void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s == &hi2s2)
    {
        process_audio_data(input_buffer, mergedFrame, 0, 0, I2S_BUF_SIZE);
        vad_preroll_process_half(1, mergedFrame, I2S_BUF_SIZE / 4);
    }
}

extern "C" void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s == &hi2s2)
    {
        process_audio_data(input_buffer, mergedFrame, I2S_BUF_SIZE, 250, I2S_BUF_SIZE);
        vad_preroll_process_half(2, mergedFrame, I2S_BUF_SIZE / 4);
    }
}

static void process_audio_data(uint16_t *inputBuffer,
                               int32_t *mergedFrameBuffer,
                               uint16_t buffer_start,
                               uint16_t merged_start,
                               uint16_t length)
{
    for (uint16_t i = 0; i < length; i += 4)
    {
        int32_t mergeValue =
            ((int32_t)(inputBuffer[i + buffer_start]) << 16) |
             (int32_t)(inputBuffer[i + buffer_start + 1]);

        mergeValue = mergeValue >> 14;
        mergedFrameBuffer[i / 4 + merged_start] = mergeValue;
    }
}