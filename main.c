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

void clean(libusb_context *ctx, libusb_device_handle *dev, int interface_0, int interface_1) {
    if (dev != NULL) {
        if (interface_0 == 0) libusb_release_interface(dev, interface_0);
        if (interface_1 == 0) libusb_release_interface(dev, interface_1);
        libusb_close(dev);
    }
    if (ctx != NULL) {
        libusb_exit(ctx);
    }
}

int init()
{
    int claim_0 = -1;
    int claim_1 = -1;
    struct uvc_probe_commit_control probe = {0};
    probe.bmHint = 0x0001;
    probe.bFormatIndex = 1;
    probe.bFrameIndex = 4;
    probe.dwFrameInterval= 0x00020EE0;
    probe.dwMaxVideoFrameSize = FRAME_SIZE;
    probe.dwMaxPayloadTransferSize = 0x0C00;
    libusb_context *ctx = NULL;
    libusb_device_handle *dev;

    int result = libusb_init(&ctx);
    printf("%d\n", result);
    if (result != 0) {
        clean(ctx, dev, claim_0, claim_1);
        return 1;
    }
    
    dev = libusb_open_device_with_vid_pid(ctx, VID, PID);
    if (dev == NULL) {
        clean(ctx, dev, claim_0, claim_1);
        return 1;
    };

    claim_0 = libusb_claim_interface(dev, 0);
    if (claim_0 != 0){
        clean(ctx, dev, claim_0, claim_1);
        return 1;
    };

    int init_crtl = esp770u_init_unknown(dev);
    printf("%d\n", init_crtl);
    if (init_crtl < 0) {
        clean(ctx, dev, claim_0, claim_1);
        return 1;
    };

    esp770u_init_radio(dev);

    int init_sensor = ar0134_init(dev);
    printf("%d\n", init_sensor);
    if (init_sensor < 0) {
        clean(ctx, dev, claim_0, claim_1);
        return 1;
    };

    int set_frame_timings = ar0134_set_timings(dev, true);
    printf("%d\n", set_frame_timings);
    if (set_frame_timings < 0) {
        clean(ctx, dev, claim_0, claim_1);
        return 1;
    };

    int set_ae_control = ar0134_set_ae(dev, true); // can be changed for manual, and other param
    printf("%d\n", set_ae_control);
    if (set_ae_control < 0) {
        clean(ctx, dev, claim_0, claim_1);
        return 1;
    };

    int set_sync = ar0134_set_sync(dev, false);
    printf("%d\n", set_sync);
    if (set_sync < 0) {
        clean(ctx, dev, claim_0, claim_1);
        return 1;
    };

    claim_1 = libusb_claim_interface(dev, 1);
    printf("%d\n", claim_1);
    if (claim_1 != 0){
        clean(ctx, dev, claim_0, claim_1);
        return 1;
    };

    int write = uvc_set_cur(dev, 1, 0, VS_PROBE_CONTROL, &probe, sizeof(probe));
    printf("%d\n", write);
    int read = uvc_get_cur(dev, 1, 0, VS_PROBE_CONTROL, &probe, sizeof(probe));
    printf("%d\n", read);
    int set = uvc_set_cur(dev, 1, 0, VS_COMMIT_CONTROL, &probe, sizeof(probe));
    printf("%d\n", set);

    int start_steaming = libusb_set_interface_alt_setting(dev, 1, 2);
    printf("%d\n", start_steaming);
    
    struct libusb_transfer *transfer = libusb_alloc_transfer(24);
    uint8_t buffer[16384*24];

    libusb_fill_iso_transfer(transfer, dev, 0x81, buffer, sizeof(buffer), 24, NULL, NULL, 5000);

    libusb_set_iso_packet_lengths(transfer, 16384);
    libusb_submit_transfer(transfer);
    libusb_handle_events_timeout_completed(ctx, &(struct timeval){.tv_sec = 0, .tv_usec = 100000}, NULL);
    libusb_handle_events_timeout_completed(ctx, &(struct timeval){.tv_sec = 0, .tv_usec = 100000}, NULL);
    /* 
    for (int i = 0; i < 50; i++) {  // 5 seconds
        libusb_handle_events_timeout_completed(ctx, &(struct timeval){.tv_sec = 0, .tv_usec = 100000}, NULL);
    }*/

    libusb_cancel_transfer(transfer);
    libusb_free_transfer(transfer);
    
    clean(ctx, dev, claim_0, claim_1);

    return 0;
}

int stream(){
    return 0;
}

int main(){
    int success_init = init();
    if (success_init == 0) stream();
}