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
#include "hls_helper/hls_helper.h"
#include <ncurses.h>

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

void filter2d_xf( uint8_t *src, rgbpxl_t *dst, uint32_t height, uint32_t width, uint32_t stridePixels, const short int coeff[KSIZE][KSIZE])
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
	 * TODO: this seems like it will break if V4L2 assigns a stride that is
	 * not equal to the active width. Might need a custom HLS function that
	 * strips away the extra bytes to handle this.
	 */
	xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> img_y(height, width, (void *) src);
	xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> img_yf(height, width);

	/*
	 * Perform the filter on the grayscale data
	 */
	xf::filter2D<XF_BORDER_CONSTANT,KSIZE,KSIZE,XF_8UC1,XF_8UC1,MAX_HEIGHT, MAX_WIDTH,XF_NPPC1>(img_y, img_yf, filter_ptr, SHIFT);

	write_output_gray(dst, img_yf, stridePixels);

}
