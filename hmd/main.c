#include <stdio.h>
#include <stdlib.h>
#include <hidapi/hidapi.h>
#include <windows.h>

#include "cv1.h"

#define HMD_VID 0x2833
#define HMD_PID 0x0031

hid_device *handle = NULL;

int main(int argc, char *argv[]) {
    int ret;
    hid_init();
/*
    struct hid_device_info *devs = hid_enumerate(HMD_VID, HMD_PID);
    struct hid_device_info *cur = devs;
    while (cur) {
        printf("usage_page: %04x usage: %04x interface: %d path: %s\n",
            cur->usage_page, cur->usage, cur->interface_number, cur->path);
        cur = cur->next;
    }
    hid_free_enumeration(devs);
    */
    
    handle =  hid_open(HMD_VID, HMD_PID, 0x00);
    if (!handle) {
        printf("failed to open HMD\n");
        return 1;
    }
    
    OuvrtDevice *dev = rift_cv1_new(NULL);
    //dev->fd = handle;

    hid_device *hid0 = hid_open_path("\\\\?\\HID#VID_2833&PID_0031&MI_01#8&40210f4&0&0000#{4d1e55b2-f16f-11cf-88cb-001111000030}");
    hid_device *hid1 = hid_open_path("\\\\?\\HID#VID_2833&PID_0031&MI_00#8&2c6cbebd&0&0000#{4d1e55b2-f16f-11cf-88cb-001111000030}");
    dev->fds[0] = hid0;
    dev->fds[1] = hid1;

    ret = rift_cv1_power_up((OuvrtRift*)dev, RIFT_CV1_POWER_DISPLAY);
    printf("power_up: %d %ls\n", ret, hid_error(handle));
    ret = rift_send_display((OuvrtRift*)dev, true, false);
    printf("display: %d %ls\n", ret, hid_error(handle));
    ret = rift_send_keepalive((OuvrtRift*)dev);
    printf("%d\n", ret);

    while (1) {
        Sleep(5000);
        rift_send_keepalive((OuvrtRift*)dev);
    }
    
    hid_close(handle);
    hid_exit();
    return 0;
}