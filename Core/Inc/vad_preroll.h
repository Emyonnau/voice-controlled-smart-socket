#ifndef VAD_PREROLL_H
#define VAD_PREROLL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void vad_preroll_process_half(int half, const int32_t *mergedFrame, uint16_t frameLength);

int vad_preroll_is_ready(void);
void vad_preroll_mark_consumed(void);

int32_t *vad_preroll_get_array(void);

#ifdef __cplusplus
}
#endif

#endif