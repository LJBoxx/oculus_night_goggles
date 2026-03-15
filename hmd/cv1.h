/*
 * Contains code from Oculus Rift HMDs
 * Copyright 2015-2016 Philipp Zabel
 * Stripped by LJBoxx
 * SPDX-License-Identifier: (LGPL-2.1-or-later OR BSL-1.0)
*/
#ifndef __CV1_H__
#define __CV1_H__

#include <glib.h>
#include <glib-object.h>
#include <stdint.h>
#include <stdbool.h>

#include "device.h"
//#include "tracker.h"

//struct tracker;

#define __cpu_to_le16(x) GUINT16_TO_LE(x)
#define __le16_to_cpu(x) GUINT16_FROM_LE(x)

#define RIFT_DISPLAY_READ_PIXEL     0x01
#define RIFT_DISPLAY_DIRECT_PENTILE  0x02

// HID Report IDs for CV1
#define RIFT_KEEPALIVE_REPORT_ID    0x08
#define RIFT_DISPLAY_REPORT_ID      0x03
#define RIFT_CV1_POWER_REPORT_ID    0x1d

// Power Flags
#define RIFT_CV1_POWER_DISPLAY      0x01

// Keepalive Settings
#define RIFT_KEEPALIVE_TIMEOUT_MS   10000
#define RIFT_KEEPALIVE_TYPE         0x01

#define OUVRT_TYPE_RIFT (ouvrt_rift_get_type())
G_DECLARE_FINAL_TYPE(OuvrtRift, ouvrt_rift, OUVRT, RIFT, OuvrtDevice)

int rift_send_keepalive(OuvrtRift *rift);
int rift_send_display(OuvrtRift *rift, bool low_persistence, bool pixel_readback);
int rift_cv1_power_up(OuvrtRift *rift, uint8_t components);
int rift_cv1_power_down(OuvrtRift *rift, uint8_t components);

//OuvrtDevice *rift_dk2_new(const char *devnode);
OuvrtDevice *rift_cv1_new(const char *devnode);

//void ouvrt_rift_set_flicker(OuvrtRift *camera, gboolean flicker);
//OuvrtTracker *ouvrt_rift_get_tracker(OuvrtRift *rift);

#endif /* __CV1_H__ */