/*
 * platform.h
 *
 *  Created on: Nov 17, 2017
 *      Author: digilent
 */

#ifndef SRC_PLATFORM_H_
#define SRC_PLATFORM_H_

#define UIO_SW_PATH "/dev/uio1"
#define UIO_GPIO_PATH "/dev/uio2"
#define UIO_DMA_PATH "/dev/uio3"
#define UIO_VTC_PATH "/dev/uio5"

#define V4L2_VIDEO_PATH "/dev/video0"
#define NUM_V4L2_BUF 8
#define V4L2_FORMAT_Y_PLANE 0

#define DISPLAY_BPP 4
#define MAX_HEIGHT 1080
#define MAX_WIDTH 1920

//#define MAX_STRIDE INPUT_FB_STRIDE

#define KSIZE 3
#define SHIFT 0


#endif /* SRC_PLATFORM_H_ */
