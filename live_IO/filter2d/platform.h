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

#define FB0_ADDR 0x10000000
#define FB1_ADDR 0x10800000
#define FB2_ADDR 0x11000000
#define FB3_ADDR 0x11800000

#define DISPLAY_BPP 4
#define MAX_HEIGHT 1080
#define MAX_WIDTH 1920
#define NUM_INPUT_FB 4

//#define MAX_STRIDE INPUT_FB_STRIDE

#define KSIZE 3
#define SHIFT 0


#endif /* SRC_PLATFORM_H_ */
