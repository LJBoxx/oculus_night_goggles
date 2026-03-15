/*
 * Contains code from Oculus Rift HMDs
 * Copyright 2015-2016 Philipp Zabel
 * Stripped by LJBoxx
 * SPDX-License-Identifier: (LGPL-2.1-or-later OR BSL-1.0)
*/
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

#include "device.h"
#include "cv1.h"

struct rift_keepalive_report {
    uint8_t id;
    uint8_t type;
    uint16_t timeout_ms;
} __attribute__((packed));

struct rift_display_report {
    uint8_t id;
    uint8_t brightness;
    uint16_t persistence;
    uint16_t total_rows;
    uint8_t flags2; // Contains the Read Pixel / Pentile flags
} __attribute__((packed));

struct rift_cv1_power_report {
    uint8_t id;
    uint8_t components;
} __attribute__((packed));

enum rift_type {
	RIFT_DK2,
	RIFT_CV1,
};

struct _OuvrtRift {
	OuvrtDevice dev;
//	OuvrtTracker *tracker;

	enum rift_type type;
//	struct leds leds;
//	vec3 imu_position;

//	unsigned char uuid[20];
//	int report_rate;
//	int report_interval;
//	gboolean flicker;
//	bool reboot;
	uint8_t boot_mode;
//	uint64_t last_message_time;
//	uint64_t last_sample_timestamp;
//	uint32_t last_exposure_timestamp;
//	int32_t last_exposure_count;
//	struct rift_radio radio;
//	struct imu_state imu;
};

struct _OuvrtRiftClass {
    OuvrtDeviceClass parent_class;
};

G_DEFINE_TYPE(OuvrtRift, ouvrt_rift, OUVRT_TYPE_DEVICE)

/*
 * Sends a keepalive report to keep the device active for 10 seconds.
 */
int rift_send_keepalive(OuvrtRift *rift)
{
    const struct rift_keepalive_report report = {
        .id = RIFT_KEEPALIVE_REPORT_ID,
        .type = RIFT_KEEPALIVE_TYPE,
        .timeout_ms = __cpu_to_le16(RIFT_KEEPALIVE_TIMEOUT_MS),
    };

    return hid_send_feature_report(rift->dev.fd, &report, sizeof(report));
}

/*
 * Sends a display report to set up low persistence and pixel readback
 * for latency measurement.
 *
int rift_send_display(OuvrtRift *rift, bool low_persistence,
			     bool pixel_readback)
{
	struct rift_display_report report = {
		.id = RIFT_DISPLAY_REPORT_ID,
	};
	uint16_t persistence;
	uint16_t total_rows;
	int ret;

	ret = hid_get_feature_report(rift->dev.fd, &report, sizeof(report));
	if (ret < 0)
		return ret;

	persistence = __le16_to_cpu(report.persistence);
	total_rows = __le16_to_cpu(report.total_rows);

	if (low_persistence) {
		report.brightness = 255;
		persistence = total_rows * 18 / 100;
	} else {
		report.brightness = 0;
		persistence = total_rows;
	}
	if (pixel_readback)
		report.flags2 |= RIFT_DISPLAY_READ_PIXEL;
	else
		report.flags2 &= ~RIFT_DISPLAY_READ_PIXEL;
	report.flags2 &= ~RIFT_DISPLAY_DIRECT_PENTILE;

	report.persistence = __cpu_to_le16(persistence);

	return hid_send_feature_report(rift->dev.fd, &report, sizeof(report));
}*/
int rift_send_display(OuvrtRift *rift, bool low_persistence, bool pixel_readback)
{
    struct rift_display_report report = {
        .id = RIFT_DISPLAY_REPORT_ID,
    };

    if (low_persistence) {
        report.brightness = 255;
        report.persistence = __cpu_to_le16(1080 * 18 / 100); // ~18% of 1080 rows
    } else {
        report.brightness = 0;
        report.persistence = __cpu_to_le16(1080);
    }

    if (pixel_readback)
        report.flags2 |= RIFT_DISPLAY_READ_PIXEL;
    else
        report.flags2 &= ~RIFT_DISPLAY_READ_PIXEL;
    report.flags2 &= ~RIFT_DISPLAY_DIRECT_PENTILE;

    return hid_send_feature_report(rift->dev.fd, &report, sizeof(report));
}

 /*
 * Powers up components of the Rift CV1.
 */
int rift_cv1_power_up(OuvrtRift *rift, uint8_t components)
{
    struct rift_cv1_power_report report = {
        .id = RIFT_CV1_POWER_REPORT_ID,
        .components = components,
    };
    return hid_send_feature_report(rift->dev.fds[0], &report, sizeof(report));
}

int rift_cv1_power_down(OuvrtRift *rift, uint8_t components)
{
	struct rift_cv1_power_report report = {
		.id = RIFT_CV1_POWER_REPORT_ID,
	};
	int ret;

	ret = hid_get_feature_report(rift->dev.fd, &report, sizeof(report));
	if (ret < 0)
		return ret;

	report.components &= ~components;

	return hid_send_feature_report(rift->dev.fd, &report, sizeof(report));
}

static void ouvrt_rift_class_init(OuvrtRiftClass *klass)
{
	G_OBJECT_CLASS(klass)->finalize = NULL;
	OUVRT_DEVICE_CLASS(klass)->start = NULL;
	OUVRT_DEVICE_CLASS(klass)->thread = NULL;
	OUVRT_DEVICE_CLASS(klass)->stop = NULL;
	OUVRT_DEVICE_CLASS(klass)->radio_start_discovery = NULL;
	OUVRT_DEVICE_CLASS(klass)->radio_stop_discovery = NULL;
}

static void ouvrt_rift_init(OuvrtRift *self)
{
	self->dev.type = DEVICE_TYPE_HMD;
    self->type = RIFT_CV1;
}

/*
 * Allocates and initializes the device structure and opens the HID device
 * file descriptor.
 *
 * Returns the newly allocated Rift device.
 */
OuvrtDevice *rift_new(enum rift_type type)
{
	OuvrtRift *rift;

	rift = g_object_new(OUVRT_TYPE_RIFT, NULL);
	if (rift == NULL)
		return NULL;

	rift->dev.has_radio = type == RIFT_CV1;
//	rift->tracker = ouvrt_tracker_new();
//	rift->type = type;

	return &rift->dev;
}

OuvrtDevice *rift_cv1_new(G_GNUC_UNUSED const char *devnode)
{
	return rift_new(RIFT_CV1);
}