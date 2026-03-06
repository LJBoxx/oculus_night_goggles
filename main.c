// i should learn cmake too x) but its fine. $$ gcc main.c uvc.c esp770u.c ar0134.c -I/c/msys64/ucrt64/include/libusb-1.0 -I/c/msys64/ucrt64/include/SDL2 -L/c/msys64/ucrt64/lib -lmingw32 -lSDL2main -lSDL2 -lusb-1.0 -o goggles
#include <stdio.h>
#include <libusb.h>
//#include <stdint.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <stdlib.h>

#include "uvc.h"
#include "esp770u.h"
#include "ar0134.h"

#define VID 0x2833
#define PID 0x0211
#define FRAME_W 1280
#define FRAME_H 960
#define FRAME_SIZE (FRAME_W * FRAME_H)
#define NUM_TRANSFERS 3

uint8_t frame_buffer[FRAME_SIZE];
int frame_ready = 0;
libusb_context *ctx = NULL;
libusb_device_handle *dev = NULL;
struct libusb_transfer *transfers[NUM_TRANSFERS];
uint8_t buffer[NUM_TRANSFERS][16384*24];
int claim_0 = -1;
int claim_1 = -1;
int running = 0;
int done_attach = 0;
int done_detach = 0;

void clean() {
    if (transfers) {
        for (int i = 0; i < NUM_TRANSFERS; i++) {
            if (transfers[i]) {
                libusb_cancel_transfer(transfers[i]);
            }
        }
    }
    if (dev != NULL) {
        if (claim_1 == 0) {
            libusb_set_interface_alt_setting(dev, 1, 0);
            libusb_release_interface(dev, claim_1);
        }
        if (claim_0 == 0) libusb_release_interface(dev, claim_0);
        libusb_close(dev);
    }
    if (ctx != NULL) {
        libusb_exit(ctx);
        SDL_Quit();
    }
}

static int LIBUSB_CALL hotplug_callback(libusb_context *ctx, libusb_device *devi, libusb_hotplug_event event, void *user_data)
{
	struct libusb_device_descriptor desc;
	libusb_device_handle *new_handle;
	int rc;

	(void)ctx;
	(void)devi;
	(void)event;
	(void)user_data;

	rc = libusb_get_device_descriptor(devi, &desc);
	if (LIBUSB_SUCCESS == rc) {
		printf ("Device attached: %04x:%04x\n", desc.idVendor, desc.idProduct);
	} else {
		printf ("Device attached\n");
		fprintf (stderr, "Error getting device descriptor: %s\n",
			 libusb_strerror((enum libusb_error)rc));
	}

	rc = libusb_open (devi, &new_handle);
	if (LIBUSB_SUCCESS == rc) {
		if (dev) {
			libusb_close (dev);
		}
		dev = new_handle;
	} else if (LIBUSB_ERROR_ACCESS != rc
#if defined(PLATFORM_WINDOWS)
		&& LIBUSB_ERROR_NOT_SUPPORTED != rc
		&& LIBUSB_ERROR_NOT_FOUND != rc
#endif
		) {
		fprintf (stderr, "No access to device: %s\n",
			 libusb_strerror((enum libusb_error)rc));
	}

	done_attach++;

	return 0;
} //from https://github.com/libusb/libusb/blob/master/examples/hotplugtest.c

static int LIBUSB_CALL hotplug_callback_detach(libusb_context *ctx, libusb_device *devi, libusb_hotplug_event event, void *user_data)
{
	struct libusb_device_descriptor desc;
	int rc;

	(void)ctx;
	(void)devi;
	(void)event;
	(void)user_data;

	rc = libusb_get_device_descriptor(devi, &desc);
	if (LIBUSB_SUCCESS == rc) {
		printf ("Device detached: %04x:%04x\n", desc.idVendor, desc.idProduct);
	} else {
		printf ("Device detached\n");
		fprintf (stderr, "Error getting device descriptor: %s\n",
			 libusb_strerror((enum libusb_error)rc));
	}

	if (dev) {
		libusb_close (dev);
		dev = NULL;
	}

	done_detach++;

	return 0;
} //from https://github.com/libusb/libusb/blob/master/examples/hotplugtest.c

void transfer_handler(struct libusb_transfer *transfer) {
    if (running == 0) return;

    static int frame_idx = 0;

    for (int i = 0; i < transfer->num_iso_packets; i++) {
        struct libusb_iso_packet_descriptor *desc = &transfer->iso_packet_desc[i];
        if (desc->status != LIBUSB_TRANSFER_COMPLETED) continue;

        // Corrected function name here:
        uint8_t *data = libusb_get_iso_packet_buffer(transfer, i);
        if (desc->actual_length < 2) continue;

        uint8_t header_len = data[0];
        uint8_t header_info = data[1];

        if (header_info & 0x02) { 
            frame_ready = 1;
            frame_idx = 0;
        }

        int payload_size = desc->actual_length - header_len;
        if (payload_size > 0 && (frame_idx + payload_size) <= FRAME_SIZE) {
            memcpy(&frame_buffer[frame_idx], data + header_len, payload_size);
            frame_idx += payload_size;
        }
    }

    libusb_submit_transfer(transfer);  //used ai to write this block it has some weird stuff going on with sensor saying its got yuyv(color) data and sending y8(greyscale)
}

int init()
{
    struct uvc_probe_commit_control probe = {0};
    probe.bmHint = 0x0001;
    probe.bFormatIndex = 1;
    probe.bFrameIndex = 4;
    probe.dwFrameInterval= 0x00020EE0;
    probe.dwMaxVideoFrameSize = FRAME_SIZE;
    probe.dwMaxPayloadTransferSize = 0x0C00;
    
    int result = libusb_init(&ctx);
    printf("%d\n", result);
    if (result != 0) {
        clean();
        return 1;
    }
    
    dev = libusb_open_device_with_vid_pid(ctx, VID, PID); //need to change that for multiple device support
    if (dev == NULL) {
        clean();
        return 1;
    }

    claim_0 = libusb_claim_interface(dev, 0);
    if (claim_0 != 0){
        clean();
        return 1;
    }

    int init_crtl = esp770u_init_unknown(dev);
    printf("%d\n", init_crtl);
    if (init_crtl < 0) {
        clean();
        return 1;
    }

    esp770u_init_radio(dev);

    int init_sensor = ar0134_init(dev);
    printf("%d\n", init_sensor);
    if (init_sensor < 0) {
        clean();
        return 1;
    }

    int set_frame_timings = ar0134_set_timings(dev, true);
    printf("%d\n", set_frame_timings);
    if (set_frame_timings < 0) {
        clean();
        return 1;
    }

    int set_ae_control = ar0134_set_ae(dev, true); // can be changed for manual, and other param
    printf("%d\n", set_ae_control);
    if (set_ae_control < 0) {
        clean();
        return 1;
    }

    int set_sync = ar0134_set_sync(dev, false);
    printf("%d\n", set_sync);
    if (set_sync < 0) {
        clean();
        return 1;
    }

    claim_1 = libusb_claim_interface(dev, 1);
    printf("%d\n", claim_1);
    if (claim_1 != 0){
        clean();
        return 1;
    }

    int write = uvc_set_cur(dev, 1, 0, VS_PROBE_CONTROL, &probe, sizeof(probe));
    printf("%d\n", write);
    int read = uvc_get_cur(dev, 1, 0, VS_PROBE_CONTROL, &probe, sizeof(probe));
    printf("%d\n", read);
    int set = uvc_set_cur(dev, 1, 0, VS_COMMIT_CONTROL, &probe, sizeof(probe));
    printf("%d\n", set);

    int start_steaming = libusb_set_interface_alt_setting(dev, 1, 2);
    printf("%d\n", start_steaming);
    
    for (int i = 0; i < NUM_TRANSFERS; i++) {
        transfers[i]= libusb_alloc_transfer(24);
        libusb_fill_iso_transfer(transfers[i], dev, 0x81, buffer[i], sizeof(buffer[i]), 24, transfer_handler, NULL, 5000);
        libusb_set_iso_packet_lengths(transfers[i], 16384);
        int ret = libusb_submit_transfer(transfers[i]);
        if (ret < 0) {
            clean();
            return 1;
        }
    }

    return 0;
}


int stream() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 1;

    SDL_Window *win = SDL_CreateWindow("Oculus Cam", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, FRAME_W, FRAME_H, 0);
    //SDL_Window *win = SDL_CreateWindow("Oculus Cam", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 960, 0);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    //SDL_SetWindowResizable(win, SDL_TRUE);
    // Use IYUV; for grayscale we only update the Y-plane (first W*H bytes)
    SDL_Texture *tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, FRAME_W, FRAME_H);

    running = 1;
    SDL_Event ev;
    while (running) {
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = 0;
        }

        struct timeval tv = {0, 10000}; // 10ms
        libusb_handle_events_timeout_completed(ctx, &tv, NULL);

        if (frame_ready) {
            void* pixels;
            int pitch;
            SDL_LockTexture(tex, NULL, &pixels, &pitch);
            memcpy(pixels, frame_buffer, FRAME_SIZE);
            memset((uint8_t*)pixels + FRAME_SIZE, 128, FRAME_SIZE / 2);
            SDL_UnlockTexture(tex);
            //SDL_UpdateTexture(tex, NULL, frame_buffer, FRAME_W);
            SDL_RenderClear(ren);
            SDL_RenderCopy(ren, tex, NULL, NULL);
            SDL_RenderPresent(ren);
            frame_ready = 0;
        }
    }
    return 0;
}

/*int main(int argc, char *argv[]) {
    if (init() == 0) stream();
    clean();
    return 0;
}*/

int main(int argc, char *argv[])
{
	libusb_hotplug_callback_handle hp[2];
	int product_id, vendor_id, class_id;
	int rc;

    vendor_id = VID;
    product_id = PID;
	//vendor_id  = (argc > 1) ? (int)strtol (argv[1], NULL, 0) : LIBUSB_HOTPLUG_MATCH_ANY;
	//product_id = (argc > 2) ? (int)strtol (argv[2], NULL, 0) : LIBUSB_HOTPLUG_MATCH_ANY;
	class_id   = (argc > 3) ? (int)strtol (argv[3], NULL, 0) : LIBUSB_HOTPLUG_MATCH_ANY;

	rc = libusb_init_context(/*ctx=*/NULL, /*options=*/NULL, /*num_options=*/0);
	if (LIBUSB_SUCCESS != rc)
	{
		printf ("failed to initialise libusb: %s\n",
			libusb_strerror((enum libusb_error)rc));
		return EXIT_FAILURE;
	}

	if (!libusb_has_capability (LIBUSB_CAP_HAS_HOTPLUG)) {
		printf ("Hotplug capabilities are not supported on this platform\n");
		libusb_exit (NULL);
		return EXIT_FAILURE;
	}

	rc = libusb_hotplug_register_callback (NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, 0, vendor_id,
		product_id, class_id, hotplug_callback, NULL, &hp[0]);
	if (LIBUSB_SUCCESS != rc) {
		fprintf (stderr, "Error registering callback 0\n");
		libusb_exit (NULL);
		return EXIT_FAILURE;
	}

	rc = libusb_hotplug_register_callback (NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, 0, vendor_id,
		product_id,class_id, hotplug_callback_detach, NULL, &hp[1]);
	if (LIBUSB_SUCCESS != rc) {
		fprintf (stderr, "Error registering callback 1\n");
		libusb_exit (NULL);
		return EXIT_FAILURE;
	}

	while (done_detach < done_attach || done_attach == 0) {
		rc = libusb_handle_events (NULL);
		if (LIBUSB_SUCCESS != rc)
			printf ("libusb_handle_events() failed: %s\n",
				libusb_strerror((enum libusb_error)rc));
	}

	if (dev) {
		printf ("Warning: Closing left-over open handle\n");
		libusb_close (dev);
	}

    if (init() == 0) stream();
    clean();

	return EXIT_SUCCESS;
} //from https://github.com/libusb/libusb/blob/master/examples/hotplugtest.c