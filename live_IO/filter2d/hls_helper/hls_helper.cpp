#include <stdlib.h>

#include "hls_helper/hls_helper.h"

/*
 * NOTE: Conversion of input stride to actual frame width is handled by all functions
 * in this library. This means the stride_pcnt provided to each function must be the stride of
 * the input or output framebuffer (frm), in number of pixels (not bytes). The width and height
 * of all Mat objects must match the active width and height of the frame.
 */

#pragma SDS data copy("inR.data"[0:"inR.size"])
#pragma SDS data access_pattern("inR.data":SEQUENTIAL)
#pragma SDS data mem_attribute("inR.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data copy("inG.data"[0:"inG.size"])
#pragma SDS data access_pattern("inG.data":SEQUENTIAL)
#pragma SDS data mem_attribute("inG.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data copy("inB.data"[0:"inB.size"])
#pragma SDS data access_pattern("inB.data":SEQUENTIAL)
#pragma SDS data mem_attribute("inB.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data copy("frm"[0:"stride_pcnt*inR.rows"])
#pragma SDS data access_pattern("frm":SEQUENTIAL)
#pragma SDS data mem_attribute("frm":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
void read_input_rgb(rgbpxl_t *frm,
		    xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> &inR,
		    xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> &inG,
		    xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> &inB,
			uint32_t stride_pcnt)
{
	int i = 0;
	for(int iY=0; iY<inR.rows; iY++){
		for(int iX=0; iX<stride_pcnt; iX++){
#pragma HLS pipeline II=1
			rgbpxl_t rgbpix = frm[i];
			if (iX < inR.cols){
				inR.data[i] = (XF_TNAME(XF_8UC1,XF_NPPC1))rgbpix.r;
				inG.data[i] = (XF_TNAME(XF_8UC1,XF_NPPC1))rgbpix.g;
				inB.data[i] = (XF_TNAME(XF_8UC1,XF_NPPC1))rgbpix.b;
			}
			i++;
		}
	}
}

#pragma SDS data copy("inGray.data"[0:"inGray.size"])
#pragma SDS data access_pattern("inGray.data":SEQUENTIAL)
#pragma SDS data mem_attribute("inGray.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data copy("frm"[0:"stride_pcnt*inGray.rows"])
#pragma SDS data access_pattern("frm":SEQUENTIAL)
#pragma SDS data mem_attribute("frm":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
void read_input_gray(rgbpxl_t *frm,
		    xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> &inGray,
			uint32_t stride_pcnt)
{
	int i = 0;
	for(int iY=0; iY<inGray.rows; iY++){
		for(int iX=0; iX<stride_pcnt; iX++){
#pragma HLS pipeline II=1
			rgbpxl_t rgbpix = frm[i];
			uint16_t rpix = (uint16_t)rgbpix.r;
			uint16_t gpix = (uint16_t)rgbpix.g;
			uint16_t bpix = (uint16_t)rgbpix.b;

			/*
			 * Grayscale is calculated by appropriately weighting the red, green, and blue pixels.
			 * The "+ 128 >> 8" portion of the code is equivalent to dividing by 256 with rounding
			 * and allows the weighting to be done with integers rather than floating or fixed point.
			 */
			XF_TNAME(XF_8UC1,XF_NPPC1) graypix = (XF_TNAME(XF_8UC1,XF_NPPC1))((rpix*76 + gpix*150 + bpix*29 + 128) >> 8);
			if (iX < inGray.cols){
				inGray.data[i] = graypix;
			}
			i++;
		}
	}
}

#pragma SDS data copy("outR.data"[0:"outR.size"])
#pragma SDS data access_pattern("outR.data":SEQUENTIAL)
#pragma SDS data mem_attribute("outR.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data copy("outG.data"[0:"outG.size"])
#pragma SDS data access_pattern("outG.data":SEQUENTIAL)
#pragma SDS data mem_attribute("outG.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data copy("outB.data"[0:"outB.size"])
#pragma SDS data access_pattern("outB.data":SEQUENTIAL)
#pragma SDS data mem_attribute("outB.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data copy("frm"[0:"stride_pcnt*outR.rows"])
#pragma SDS data access_pattern("frm":SEQUENTIAL)
#pragma SDS data mem_attribute("frm":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
void write_output_rgb(rgbpxl_t *frm,
		    xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> &outR,
		    xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> &outG,
		    xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> &outB,
			uint32_t stride_pcnt)
{
	int i = 0;
	for(int iY=0; iY<outR.rows; iY++){
		for(int iX=0; iX<stride_pcnt; iX++){
#pragma HLS pipeline II=1
			if (iX < outR.cols){
				rgbpxl_t rgbpix;
				rgbpix.r = (unsigned char)(outR.data[i]);
				rgbpix.g = (unsigned char)(outG.data[i]);
				rgbpix.b = (unsigned char)(outB.data[i]);
				rgbpix.dummy = 0;
				frm[i] = rgbpix;
			}
			else{
				rgbpxl_t rgbpix_zero = {0,0,0,0};
				frm[i] = rgbpix_zero;
			}
			i++;
		}
	}
}

#pragma SDS data copy("outGray.data"[0:"outGray.size"])
#pragma SDS data access_pattern("outGray.data":SEQUENTIAL)
#pragma SDS data mem_attribute("outGray.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data copy("frm"[0:"stride_pcnt*outGray.rows"])
#pragma SDS data access_pattern("frm":SEQUENTIAL)
#pragma SDS data mem_attribute("frm":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
void write_output_gray(rgbpxl_t *frm,
		    xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, XF_NPPC1> &outGray,
			uint32_t stride_pcnt)
{
	int i = 0;
	for(int iY=0; iY<outGray.rows; iY++){
		for(int iX=0; iX<stride_pcnt; iX++){
#pragma HLS pipeline II=1
			if (iX < outGray.cols){
				unsigned char graypix = (unsigned char)(outGray.data[i]);
				rgbpxl_t rgbpix;
				rgbpix.r = graypix;
				rgbpix.g = graypix;
				rgbpix.b = graypix;
				rgbpix.dummy = 0;
				frm[i] = rgbpix;
			}
			else{
				rgbpxl_t rgbpix_zero = {0,0,0,0};
				frm[i] = rgbpix_zero;
			}
			i++;
		}
	}
}

