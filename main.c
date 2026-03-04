// i should learn cmake too x) but its fine. $ gcc main.c uvc.c esp770u.c ar0134.c -I/c/msys64/ucrt64/include/libusb-1.0 -L/c/msys64/ucrt64/lib -lusb-1.0 -o goggles
#include <stdio.h>
#include <libusb.h>
//#include <stdint.h>
#include <string.h>

#include "uvc.h"
#include "esp770u.h"
#include "ar0134.h"

#define VID 0x2833
#define PID 0x0211
#define FRAME_W 1280
#define FRAME_H 960
#define FRAME_SIZE (FRAME_W * FRAME_H)

uint8_t frame_buffer[FRAME_SIZE];
int frame_ready = 0;


int init()
{
    int error = 0;
    int claim_0;
    int claim_1;
    libusb_context *ctx = NULL;
    libusb_device_handle *dev;

    int result = libusb_init(&ctx);
    if (result != 0) {
        error = 1;
        goto cleanup;
    }
    //printf("%d\n", result);
    
    dev = libusb_open_device_with_vid_pid(ctx, VID, PID);
    if (dev == NULL) {
        error = 1;
        goto cleanup;
    };

    claim_0 = libusb_claim_interface(dev, 0);
    if (claim_0 != 0){
        error = 1;
        goto cleanup;
    };

    int init_crtl = esp770u_init_unknown(dev);
    printf("%d\n", init_crtl);
    if (init_crtl < 0) {
        error = 1;
        goto cleanup;
    };

    int init_sensor = ar0134_init(dev);
    printf("%d\n", init_sensor);
    if (init_sensor < 0) {
        error = 1;
        goto cleanup;
    };

    int set_frame_timings = ar0134_set_timings(dev, true);
    printf("%d\n", set_frame_timings);
    if (set_frame_timings < 0) {
        error = 1;
        goto cleanup;
    };

    int set_ae_control = ar0134_set_ae(dev, true); // can be changed for manual, and other param
    printf("%d\n", set_ae_control);
    if (set_ae_control < 0) {
        error = 1;
        goto cleanup;
    };

    int set_sync = ar0134_set_sync(dev, true);
    printf("%d\n", set_sync);
    if (set_sync < 0) {
        error = 1;
        goto cleanup;
    };

    claim_1 = libusb_claim_interface(dev, 1);
    if (claim_1 != 0){
        error = 1;
        goto cleanup;
    };
    
cleanup:
    if (dev != NULL) {
        if (claim_0 == 0) libusb_release_interface(dev, 0);
        if (claim_1 == 0) libusb_release_interface(dev, 1);
        libusb_close(dev);
    }
    if (ctx != NULL) {
        libusb_exit(ctx);
    }
    
    return error;
}

int strem(){
    
}

int main(){
    int success_init = init();
    if (success_init == 0) stream();

}