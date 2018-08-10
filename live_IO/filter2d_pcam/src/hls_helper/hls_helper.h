#ifndef SRC_HLS_HELPER_H_
#define SRC_HLS_HELPER_H_

#include <common/xf_common.h>
#include "rgbpxl.h"
/*
 * The project must include a file called platform.h that #defines the following:
 * MAX_WIDTH - The maximum allowed frame width, used for xfopencv template instantiations
 * MAX_HEIGHT - The maximum allowed frame height, used for xfopencv template instantiations
 */
#include "platform.h"

void read_input_rgb(rgbpxl_t *frm,
		    xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> &inR,
		    xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> &inG,
		    xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> &inB,
			uint32_t stride_pcnt);

void read_input_gray(rgbpxl_t *frm,
		    xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> &inGray,
			uint32_t stride_pcnt);

void write_output_rgb(rgbpxl_t *frm,
		    xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> &outR,
		    xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> &outG,
		    xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> &outB,
			uint32_t stride_pcnt);

void write_output_gray(rgbpxl_t *frm,
		    xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> &outGray,
			uint32_t stride_pcnt);


#endif
