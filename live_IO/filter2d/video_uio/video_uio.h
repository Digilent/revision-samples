#ifndef SRC_VIDEO_UIO_H_
#define SRC_VIDEO_UIO_H_

#include <stdint.h>

#define VIDEO_UIO_SUCCESS 0
#define VIDEO_UIO_FAILURE -1

#define INPUT_DETECT_TIMEOUT 5

int video_uio_detect(char const *gpioUioPath, char const *vtcUioPath, uint32_t *hDetect, uint32_t *vDetect);
void video_uio_start(char const *uioDmaPath, uint32_t fb_addr, uint32_t hActiveIn, uint32_t vActiveIn, uint32_t bytesPerPxl, uint32_t stride);

#endif
