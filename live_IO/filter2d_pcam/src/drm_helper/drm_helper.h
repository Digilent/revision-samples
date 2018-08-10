/*
 * drm_helper.h
 *
 *  Created on: Nov 8, 2017
 *      Author: digilent
 */

#ifndef SRC_DRM_HELPER_H_
#define SRC_DRM_HELPER_H_

#include <fcntl.h>
#include <sys/mman.h>
#include <drm/drm.h>
#include <drm/drm_mode.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#define FAILURE     -1
#define SUCCESS     0

#define NUM_DUMB_BUFFERS     3
#define NUM_MAX_RESOURCE_IDS 10
#define NUM_MAX_MODES        60

typedef struct drm_cntrl{
    int dri_fd;

    struct drm_mode_modeinfo current;     // The current modeset of the CRTC
    void * fbMem[NUM_DUMB_BUFFERS];         // memory mapped locations of dumb buffers
    uint8_t current_fb;                     // between 0 and NUM_DUMB_BUFFERS, the current fb that is mapped to the CRTC

    struct drm_mode_card_res mode_card;       // Resolution mode card
    struct drm_mode_get_connector connector;  // Connector information
    struct drm_mode_get_encoder enc;          // Encoder information
    struct drm_mode_crtc crtc;                // CRTC information

    // For the dumb buffers
    struct drm_mode_create_dumb create_dumb[NUM_DUMB_BUFFERS];
    struct drm_mode_map_dumb map_dumb[NUM_DUMB_BUFFERS];
    struct drm_mode_fb_cmd cmd_dumb[NUM_DUMB_BUFFERS];
    struct drm_prime_handle prime[NUM_DUMB_BUFFERS];

    //Resource IDs
    uint64_t res_fb_buf[NUM_MAX_RESOURCE_IDS];
    uint64_t res_crtc_buf[NUM_MAX_RESOURCE_IDS];
    uint64_t res_conn_buf[NUM_MAX_RESOURCE_IDS];
    uint64_t res_enc_buf[NUM_MAX_RESOURCE_IDS];

    //Mode information
    struct drm_mode_modeinfo mode_buffer[NUM_MAX_MODES];     // Will populate with the available modes given by the monitor
    uint64_t conn_prop_buf[NUM_MAX_MODES];
    uint64_t conn_propval_buf[NUM_MAX_MODES];
    uint64_t conn_enc_buf[NUM_MAX_MODES];

} drm_cntrl;


int mapFBtoCRTC(drm_cntrl * control, int fb_num);
int setNewResolution(char const * port, struct drm_mode_modeinfo * mode, drm_cntrl * control );
int drmControlInit(char const * port, uint32_t width, uint32_t height, drm_cntrl * control );
int openPort(char const * port, int * fd);
int drmControlClose(drm_cntrl * control);


#endif
