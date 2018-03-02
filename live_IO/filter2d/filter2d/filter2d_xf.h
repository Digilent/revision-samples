/*
 * filter2d_xf.h
 *
 *  Created on: Nov 21, 2017
 *      Author: digilent
 */

#ifndef SRC_FILTER2D_FILTER2D_XF_H_
#define SRC_FILTER2D_FILTER2D_XF_H_

#include "platform.h"
#include "common/xf_common.h"

void filter2d_xf( xf::Mat<XF_8UC4,  MAX_HEIGHT, MAX_WIDTH, XF_NPPC1>  *src, xf::Mat<XF_8UC4,  MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> *dst, const short int coeff[KSIZE][KSIZE]);

#endif /* SRC_FILTER2D_FILTER2D_XF_H_ */
