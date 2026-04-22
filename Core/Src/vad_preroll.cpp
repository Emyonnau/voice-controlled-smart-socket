#include "vad_preroll.h"
#include "app_config.h"
#include "debug_uart.h"

#include "main.h"

#include <stdlib.h>
#include <stdint.h>

static int32_t arrayBuffer[ARRAY_SIZE];
static unsigned int idx = 0;

static int array_ready = 0;
static int start_recording = 0;
static int pre_recording = 1;

static int32_t ring_buffer[PRE_RECORD_SIZE];
static unsigned int pre_record_idx = 0;

static int delay_counter = 0;
static uint32_t anfangZeit = 0;
static uint32_t endeZeit = 0;

static int32_t averageVal(void);

void vad_preroll_process_half(int half, const int32_t *mergedFrame, uint16_t frameLength)
{
    delay_counter += 125;

    int maxVal = -999999999;
    int minVal =  999999999;

    /* Ein paar Sekunden warten auf sinnvolle Werte */
    if (delay_counter <= 3200)
    {
        return;
    }

    delay_counter = 4000;

    /* Pre-recording ring buffer füllen */
    if (pre_recording == 1)
    {
        if (pre_record_idx < (PRE_RECORD_SIZE - frameLength))
        {
            if (half == 1)
            {
                for (unsigned int j = 0; j < frameLength; j++)
                {
                    ring_buffer[j + pre_record_idx] = mergedFrame[j];
                }
            }
            else if (half == 2)
            {
                for (unsigned int j = 0; j < frameLength; j++)
                {
                    ring_buffer[j + pre_record_idx] = mergedFrame[j + frameLength];
                }
            }

            pre_record_idx += frameLength;
        }
        else
        {
            pre_record_idx = 0;

            if (half == 1)
            {
                for (unsigned int j = 0; j < frameLength; j++)
                {
                    ring_buffer[j + pre_record_idx] = mergedFrame[j];
                }
            }
            else if (half == 2)
            {
                for (unsigned int j = 0; j < frameLength; j++)
                {
                    ring_buffer[j + pre_record_idx] = mergedFrame[j + frameLength];
                }
            }

            pre_record_idx += frameLength;
        }
    }

    /* Überprüfung, ob threshold überschritten wird */
    if (half == 1)
    {
        for (unsigned int i = 0; i < frameLength; i++)
        {
            if (mergedFrame[i] > maxVal)
            {
                maxVal = mergedFrame[i];
            }

            if (mergedFrame[i] < minVal)
            {
                minVal = mergedFrame[i];
            }
        }
    }
    else if (half == 2)
    {
        for (unsigned int i = frameLength; i < (frameLength * 2U); i++)
        {
            if (mergedFrame[i] > maxVal)
            {
                maxVal = mergedFrame[i];
            }

            if (mergedFrame[i] < minVal)
            {
                minVal = mergedFrame[i];
            }
        }
    }

    int32_t adaptiverSchwellwert = averageVal();

    if (adaptiverSchwellwert == 0)
    {
        adaptiverSchwellwert = THRESHOLD;
    }

    /* Wenn threshold überschritten wird, wird start flag 1 */
    if (maxVal > adaptiverSchwellwert || minVal < -adaptiverSchwellwert)
    {
        start_recording = 1;
        pre_recording = 0;
    }
    else
    {
        start_recording = 0;
    }

    /* Aufzeichnung beginnt */
    if (start_recording == 1)
    {
        array_ready = 0;
        anfangZeit = HAL_GetTick();

        int j = 0;

        for (unsigned int i = pre_record_idx; i < PRE_RECORD_SIZE; i++)
        {
            arrayBuffer[j] = ring_buffer[i];
            j++;
        }

        for (unsigned int i = 0; i < pre_record_idx; i++)
        {
            arrayBuffer[j] = ring_buffer[i];
            j++;
        }

        /* Speicherung der Werte im 16000 großen Array */
        if (idx < 12000)
        {
            if (half == 1)
            {
                for (unsigned int j2 = 0; j2 < frameLength; j2++)
                {
                    arrayBuffer[j2 + idx + PRE_RECORD_SIZE] = mergedFrame[j2];
                    DisplayVolume(mergedFrame[j2]);
                }
            }
            else if (half == 2)
            {
                for (unsigned int j2 = 0; j2 < frameLength; j2++)
                {
                    arrayBuffer[j2 + idx + PRE_RECORD_SIZE] = mergedFrame[j2 + frameLength];
                    DisplayVolume(mergedFrame[j2 + frameLength]);
                }
            }

            idx += frameLength;
        }
        else
        {
            /* Speicherung endet */
            idx = 0;
            pre_record_idx = 0;
            start_recording = 0;
            endeZeit = HAL_GetTick() - anfangZeit;
            (void)endeZeit;

            array_ready = 1;
            pre_recording = 1;
        }
    }
}

int vad_preroll_is_ready(void)
{
    return array_ready;
}

void vad_preroll_mark_consumed(void)
{
    array_ready = 0;
}

int32_t *vad_preroll_get_array(void)
{
    return arrayBuffer;
}

static int32_t averageVal(void)
{
    if (array_ready == 1)
    {
        int32_t sum = 0;

        for (int i = 0; i < ARRAY_SIZE - 1; i++)
        {
            sum += abs(arrayBuffer[i]);
        }

        return sum / ARRAY_SIZE;
    }
    else
    {
        return 0;
    }
}