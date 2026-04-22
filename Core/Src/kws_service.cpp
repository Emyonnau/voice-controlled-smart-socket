#include "kws_service.h"
#include "app_config.h"
#include "vad_preroll.h"
#include "socket_control.h"

#include "mfcc.h"
#include "kws.h"

#include <stdio.h>
#include <string>

static MFCC* mfcc = nullptr;
static KWS* kws = nullptr;

void kws_service_init(void)
{
    if (mfcc == nullptr)
    {
        mfcc = new MFCC(MFCC_OUT_SIZE, MFCC_IN_SIZE, 2);
    }

    if (kws == nullptr)
    {
        kws = new KWS();
    }
}

void kws_service_process(void)
{
    if (vad_preroll_is_ready() != 1)
    {
        return;
    }

    if (mfcc == nullptr || kws == nullptr)
    {
        return;
    }

    int32_t *array = vad_preroll_get_array();
    vad_preroll_mark_consumed();

    q7_t mfccOutTmp[MFCC_OUT_SIZE];

    printf("Feature extraction started\r\n");

    int feature_counter = 0;

    for (int i = 0; i <= ARRAY_SIZE - MFCC_IN_SIZE; i += MFCC_STEP_SIZE, feature_counter++)
    {
        float mfccInputFrame[MFCC_IN_SIZE];

        for (int j = 0; j < MFCC_IN_SIZE; j++)
        {
            mfccInputFrame[j] = (float)array[i + j];
        }

        mfcc->mfcc_compute(mfccInputFrame, mfccOutTmp);

        for (int k = 0; k < MFCC_OUT_SIZE; k++)
        {
            kws->mMFCC[feature_counter][k] = (ai_float)mfccOutTmp[k];
        }
    }

    printf("Feature extraction ended\r\n");

    int index = kws->runInference();
    std::string word = kws->indexToWord(index);

    printf("%s\r\n", word.c_str());

    if (word == "YES")
    {
        toggle_socket(1);
    }
    else if (word == "TWO")
    {
        toggle_socket(0);
    }
}