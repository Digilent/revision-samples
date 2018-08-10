/*
 * filter2d_xf.h
 *
 *  Created on: Nov 21, 2017
 *      Author: digilent
 */

#ifndef SRC_FILTER2D_FILTER2D_XF_H_
#define SRC_FILTER2D_FILTER2D_XF_H_

#include "platform.h"
#include "rgbpxl.h"
#include "common/xf_common.h"

void filter2d_xf( uint8_t *src, rgbpxl_t *dst, uint32_t height, uint32_t width, uint32_t stridePixels, const short int coeff[KSIZE][KSIZE]);

#endif /* SRC_FILTER2D_FILTER2D_XF_H_ */
