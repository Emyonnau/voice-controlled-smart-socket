#include "app.h"
#include "audio_capture.h"
#include "kws_service.h"
#include "socket_control.h"

void app_init(void)
{
    socket_control_init();
    kws_service_init();
    audio_capture_init();
    audio_capture_start();
}

void app_process(void)
{
    kws_service_process();
}