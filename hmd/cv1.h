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