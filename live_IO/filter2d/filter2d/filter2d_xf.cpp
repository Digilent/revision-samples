/******************************************************************************
 *
 * (c) Copyright 2012-2016 Xilinx, Inc. All rights reserved.
 *
 * This file contains confidential and proprietary information of Xilinx, Inc.
 * and is protected under U.S. and international copyright and other
 * intellectual property laws.
 *
 * DISCLAIMER
 * This disclaimer is not a license and does not grant any rights to the
 * materials distributed herewith. Except as otherwise provided in a valid
 * license issued to you by Xilinx, and to the maximum extent permitted by
 * applicable law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND WITH ALL
 * FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS, EXPRESS,
 * IMPLIED, OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF
 * MERCHANTABILITY, NON-INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE;
 * and (2) Xilinx shall not be liable (whether in contract or tort, including
 * negligence, or under any other theory of liability) for any loss or damage
 * of any kind or nature related to, arising under or in connection with these
 * materials, including for any direct, or any indirect, special, incidental,
 * or consequential loss or damage (including loss of data, profits, goodwill,
 * or any type of loss or damage suffered as a result of any action brought by
 * a third party) even if such damage or loss was reasonably foreseeable or
 * Xilinx had been advised of the possibility of the same.
 *
 * CRITICAL APPLICATIONS
 * Xilinx products are not designed or intended to be fail-safe, or for use in
 * any application requiring fail-safe performance, such as life-support or
 * safety devices or systems, Class III medical devices, nuclear facilities,
 * applications related to the deployment of airbags, or any other applications
 * that could lead to death, personal injury, or severe property or
 * environmental damage (individually and collectively, "Critical
 * Applications"). Customer assumes the sole risk and liability of any use of
 * Xilinx products in Critical Applications, subject only to applicable laws
 * and regulations governing limitations on product liability.
 *
 * THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS PART OF THIS FILE
 * AT ALL TIMES.
 *
 *******************************************************************************/

#include "filter2d_xf.h"
#include "platform.h"

#include"hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"
#include "imgproc/xf_cvt_color.hpp"
#include "imgproc/xf_custom_convolution.hpp"
#include "imgproc/xf_delay.hpp"
#include "imgproc/xf_channel_combine.hpp"
#include "imgproc/xf_channel_extract.hpp"
#include "imgproc/xf_duplicateimage.hpp"

void filter2d_xf( xf::Mat<XF_8UC4,  MAX_HEIGHT, MAX_WIDTH, XF_NPPC1>  *src, xf::Mat<XF_8UC4,  MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> *dst, const short int coeff[KSIZE][KSIZE])
{
	short int filter_ptr[KSIZE*KSIZE];
	for(int i = 0; i < KSIZE; i++)
	{
		for(int j = 0; j < KSIZE; j++)
		{
			filter_ptr[i*KSIZE+j] = coeff[i][j];
		}
	}

	/*
	 * Extract the luminance from the rgb data to use as the grayscale date
	 */
	xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> img_y(src->rows, src->cols);
	xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> img_yf(src->rows, src->cols);
	xf::Mat<XF_8UC2, MAX_HEIGHT/2, MAX_WIDTH/2, XF_NPPC1> img_uv(src->rows/2, src->cols/2);
	xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> img_yfDup11(src->rows, src->cols);
	xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> img_yfDup12(src->rows, src->cols);
	xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> img_yfDup21(src->rows, src->cols);
	xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> img_yfDup22(src->rows, src->cols);
	xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> img_yfDup23(src->rows, src->cols);
	xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> img_yfDup24(src->rows, src->cols);


	xf::rgba2nv12<XF_8UC4, XF_8UC1, XF_8UC2, MAX_HEIGHT, MAX_WIDTH,XF_NPPC1 >(*src, img_y, img_uv);

	/*
	 * Perform the filter on the grayscale data
	 */
	xf::filter2D<XF_BORDER_CONSTANT,KSIZE,KSIZE,XF_8UC1,XF_8UC1,MAX_HEIGHT, MAX_WIDTH,XF_NPPC1>(img_y, img_yf, filter_ptr, SHIFT);

	/*
	 * Preserve SDSoC dataflow by explicitly duplicating filtered buffer. This prevents the data from being woven into and out of DDR and keeps
	 * it in the fabric.
	 */
	xf::Duplicatemats<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1>(img_yf, img_yfDup11, img_yfDup12);
	xf::Duplicatemats<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1>(img_yfDup11, img_yfDup21, img_yfDup22);
	xf::Duplicatemats<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1>(img_yfDup12, img_yfDup23, img_yfDup24);

	/*
	 * Use the grayscale data for the R,G, and B channels of the output pipeline. It is also used for the 4th unused channel of the output pipeline
	 * because something has to be used and it is less wasteful than creating and using a blank mat object.
	 */
	xf::merge<XF_8UC1, XF_8UC4, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1>(img_yfDup21, img_yfDup22, img_yfDup23, img_yfDup24, *dst);

/*
 * This is an alternate method that can be used to attempt to recombine in the original color.
 * It works fine for some filters, but colors "bleed" through low luminance areas, so the
 * filters that introduce a lot of black (such as "off" or edge detection) will be very off.
 * Don't forget to swap out the hardware accelerated functions if you use this.
 */
//	xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> img_y(src->rows, src->cols);
//	xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> img_yf(src->rows, src->cols);
//	xf::Mat<XF_8UC2, MAX_HEIGHT/2, MAX_WIDTH/2, XF_NPPC1> img_uv(src->rows/2, src->cols/2);
//	xf::Mat<XF_8UC2, MAX_HEIGHT/2, MAX_WIDTH/2, XF_NPPC1> img_uvDelay(src->rows/2, src->cols/2);
//	xf::rgba2nv12<XF_8UC4, XF_8UC1, XF_8UC2, MAX_HEIGHT, MAX_WIDTH,XF_NPPC1 >(*src, img_y, img_uv);
//	xf::filter2D<XF_BORDER_CONSTANT,KSIZE,KSIZE,XF_8UC1,XF_8UC1,MAX_HEIGHT, MAX_WIDTH,XF_NPPC1>(img_y, img_yf, filter_ptr, SHIFT);
//	xf::delayimages<XF_8UC2, MAX_HEIGHT/2, MAX_WIDTH/2, XF_NPPC1, MAX_WIDTH>(img_uv, img_uvDelay);
//	xf::nv122rgba<XF_8UC1, XF_8UC2, XF_8UC4,MAX_HEIGHT,MAX_WIDTH,XF_NPPC1>(img_yf,img_uvDelay,*dst);

/*
 * Another alternate method that runs a filter on each of the red, green, and blue channels.
 * This uses a lot of resources but preserves color correctly.
 */
//	xf::Mat<XF_8UC4, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> img_srcDup11(src->rows, src->cols);
//	xf::Mat<XF_8UC4, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> img_srcDup12(src->rows, src->cols);
//	xf::Mat<XF_8UC4, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> img_srcDup21(src->rows, src->cols);
//	xf::Mat<XF_8UC4, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> img_srcDup22(src->rows, src->cols);
//	xf::Mat<XF_8UC4, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> img_srcDup23(src->rows, src->cols);
//	xf::Mat<XF_8UC4, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> img_srcDup24(src->rows, src->cols);
//	xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> img_rIn(src->rows, src->cols);
//	xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> img_gIn(src->rows, src->cols);
//	xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> img_bIn(src->rows, src->cols);
//	xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> img_aIn(src->rows, src->cols);
//	xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> img_rOut(src->rows, src->cols);
//	xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> img_gOut(src->rows, src->cols);
//	xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> img_bOut(src->rows, src->cols);
//	xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> img_aOut(src->rows, src->cols);
//
//	xf::Duplicatemats<XF_8UC4, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1>(*src, img_srcDup11, img_srcDup12);
//	xf::Duplicatemats<XF_8UC4, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1>(img_srcDup11, img_srcDup21, img_srcDup22);
//	xf::Duplicatemats<XF_8UC4, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1>(img_srcDup12, img_srcDup23, img_srcDup24);
//
//	xf::extractChannel<XF_8UC4, XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1>(img_srcDup21,img_rIn,0);
//	xf::extractChannel<XF_8UC4, XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1>(img_srcDup22,img_gIn,1);
//	xf::extractChannel<XF_8UC4, XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1>(img_srcDup23,img_bIn,2);
//	xf::extractChannel<XF_8UC4, XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1>(img_srcDup24,img_aIn,3);
//
//	xf::filter2D<XF_BORDER_CONSTANT,KSIZE,KSIZE,XF_8UC1,XF_8UC1,MAX_HEIGHT, MAX_WIDTH,XF_NPPC1>(img_rIn, img_rOut, filter_ptr, SHIFT);
//	xf::filter2D<XF_BORDER_CONSTANT,KSIZE,KSIZE,XF_8UC1,XF_8UC1,MAX_HEIGHT, MAX_WIDTH,XF_NPPC1>(img_gIn, img_gOut, filter_ptr, SHIFT);
//	xf::filter2D<XF_BORDER_CONSTANT,KSIZE,KSIZE,XF_8UC1,XF_8UC1,MAX_HEIGHT, MAX_WIDTH,XF_NPPC1>(img_bIn, img_bOut, filter_ptr, SHIFT);
//	xf::delayimages<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1,2*MAX_WIDTH>(img_aIn, img_aOut);
//
//	xf::merge<XF_8UC1, XF_8UC4, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1>(img_bOut, img_gOut, img_rOut, img_aOut, *dst);


}
