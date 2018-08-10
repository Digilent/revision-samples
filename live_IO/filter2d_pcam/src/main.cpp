/*
 * main.c
 *
 *  Created on: Nov 8, 2017
 *      Author: digilent
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <ncurses.h>
#include "filter2d/filter2d_int.h"
#include "filter2d/filter2d_xf.h"
#include "drm_helper/drm_helper.h"
#include "platform.h"
#include "uio/uio_ll.h"
#include "common/xf_common.h"
#include "sds_lib.h"
#include "rgbpxl.h"
#include "v4l2_helper/v4l2_helper.h"

int main(int argc, char **argv)
{
    uint32_t hActiveIn, vActiveIn;
    int sw_fd, tty_fd, mem_fd;;
    uint8_t *swMem;
    struct drm_cntrl drm = {0};
    struct v4l2_helper v4l2_help = {0};

    /*
     * Initialize V4L2 input and allocate buffers. Note that the resolution
     * is controlled by calling media-ctl prior to launching this program.
     * Also, v4l2-ctl must set the format to NM16 and yavta must be run at
     * least for a single frame with the equivalent format specifier (NV16M).
     */
    v4l2_helper_open(&v4l2_help, V4L2_VIDEO_PATH);
    v4l2_helper_init(&v4l2_help, NUM_V4L2_BUF);

    for (int i = 0; i < v4l2_help.num_buf; i++)
    	for (int j = 0; j < v4l2_help.fmt.fmt.pix_mp.num_planes; j++)
    		sds_register_dmabuf(v4l2_help.buffers[i].start[j],v4l2_help.buffers[i].dmabuf_fd[j]);

    hActiveIn = v4l2_help.fmt.fmt.pix_mp.width;
    vActiveIn = v4l2_help.fmt.fmt.pix_mp.height;

    /*
     * Stop the terminal from being drawn on the monitor
     */
	tty_fd = open("/dev/tty1", O_RDWR);
	ioctl(tty_fd,KDSETMODE,KD_GRAPHICS);
	close(tty_fd);

	/*
	 * Take control of the display device, set the resolution to match the input, and create
	 * multiple framebuffers.
	 */
	if (drmControlInit("/dev/dri/card0",hActiveIn,vActiveIn,&drm) != SUCCESS)
	{
		printf("drmControlInit Failed");
		return -1;
	}

	/*
	 * Set Switches and buttons as input
	 */
    sw_fd = open(UIO_SW_PATH, O_RDWR);
	swMem = (uint8_t *) mmap(0, UIO_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, sw_fd, (off_t)0);
	UioWrite32(swMem, 4, 0xF); //set all 4 switches as input
	UioWrite32(swMem, 12, 0xF); //set all 4 buttons as input

	/*
	 * Initialize ncurses for framerate display and non-blocking user input
	 */
	WINDOW *mywin = initscr();
	cbreak();
	nodelay(mywin, TRUE);
	wmove(mywin, 0, 0);
	printw("Filter2d is now running on the attached display\n");
	printw("Use the 4 onboard switches to control the filter\n");
	printw("Input Resolution: %dx%d\n",hActiveIn,vActiveIn);
	printw("Output Resolution: %dx%d\n",drm.current.hdisplay,drm.current.vdisplay);
	printw("DRM FB Stride (bytes): %d\n",drm.create_dumb[drm.current_fb].pitch);
	printw("Input Plane 0 Stride (bytes): %d\n",v4l2_help.fmt.fmt.pix_mp.plane_fmt[0].bytesperline);
	printw("Input Plane 1 Stride (bytes): %d\n",v4l2_help.fmt.fmt.pix_mp.plane_fmt[1].bytesperline);
	printw("\n");
	printw("Press any key to exit...");
	wrefresh(mywin);

	/*
	 * Start capturing frames from V4L2 device
	 */
	v4l2_helper_start_cap(&v4l2_help);

	unsigned long long ticks = sds_clock_counter();
	unsigned long long timeElapsed;
	int i = 0;
	double frameRate = 0;
	int userInput = ERR;
	while (userInput == ERR)
	{
		uint32_t sw = UioRead32(swMem, 0);
		uint32_t btn = UioRead32(swMem, 8);
		unsigned int v4l2_buf_index;

		v4l2_buf_index = v4l2_helper_readframe(&v4l2_help);
//		v4l2_buf_index = 0;
//		while (!v4l2_buf_index)
//		{
//			v4l2_buf_index = v4l2_helper_dequeue(&v4l2_help);
//		}

		/*
		 * TODO: Software Opencv calls are currently broken
		 */
		if (btn & 1)
		{
			switch (sw)
			{
			case 0 :
				filter2d_cv((uint8_t *) v4l2_help.buffers[v4l2_buf_index].start[V4L2_FORMAT_Y_PLANE],(uint32_t *) drm.fbMem[drm.current_fb],vActiveIn, hActiveIn, v4l2_help.fmt.fmt.pix_mp.plane_fmt[V4L2_FORMAT_Y_PLANE].bytesperline, drm.create_dumb[drm.current_fb].pitch, coeff_off);
				break;
			case 2 :
				filter2d_cv((uint8_t *) v4l2_help.buffers[v4l2_buf_index].start[V4L2_FORMAT_Y_PLANE],(uint32_t *) drm.fbMem[drm.current_fb],vActiveIn, hActiveIn, v4l2_help.fmt.fmt.pix_mp.plane_fmt[V4L2_FORMAT_Y_PLANE].bytesperline, drm.create_dumb[drm.current_fb].pitch, coeff_blur);
				break;
			case 3 :
				filter2d_cv((uint8_t *) v4l2_help.buffers[v4l2_buf_index].start[V4L2_FORMAT_Y_PLANE],(uint32_t *) drm.fbMem[drm.current_fb],vActiveIn, hActiveIn, v4l2_help.fmt.fmt.pix_mp.plane_fmt[V4L2_FORMAT_Y_PLANE].bytesperline, drm.create_dumb[drm.current_fb].pitch, coeff_edge);
				break;
			case 4 :
				filter2d_cv((uint8_t *) v4l2_help.buffers[v4l2_buf_index].start[V4L2_FORMAT_Y_PLANE],(uint32_t *) drm.fbMem[drm.current_fb],vActiveIn, hActiveIn, v4l2_help.fmt.fmt.pix_mp.plane_fmt[V4L2_FORMAT_Y_PLANE].bytesperline, drm.create_dumb[drm.current_fb].pitch, coeff_edge_h);
				break;
			case 5 :
				filter2d_cv((uint8_t *) v4l2_help.buffers[v4l2_buf_index].start[V4L2_FORMAT_Y_PLANE],(uint32_t *) drm.fbMem[drm.current_fb],vActiveIn, hActiveIn, v4l2_help.fmt.fmt.pix_mp.plane_fmt[V4L2_FORMAT_Y_PLANE].bytesperline, drm.create_dumb[drm.current_fb].pitch, coeff_edge_v);
				break;
			case 6 :
				filter2d_cv((uint8_t *) v4l2_help.buffers[v4l2_buf_index].start[V4L2_FORMAT_Y_PLANE],(uint32_t *) drm.fbMem[drm.current_fb],vActiveIn, hActiveIn, v4l2_help.fmt.fmt.pix_mp.plane_fmt[V4L2_FORMAT_Y_PLANE].bytesperline, drm.create_dumb[drm.current_fb].pitch, coeff_emboss);
				break;
			case 7 :
				filter2d_cv((uint8_t *) v4l2_help.buffers[v4l2_buf_index].start[V4L2_FORMAT_Y_PLANE],(uint32_t *) drm.fbMem[drm.current_fb],vActiveIn, hActiveIn, v4l2_help.fmt.fmt.pix_mp.plane_fmt[V4L2_FORMAT_Y_PLANE].bytesperline, drm.create_dumb[drm.current_fb].pitch, coeff_gradient_h);
				break;
			case 8 :
				filter2d_cv((uint8_t *) v4l2_help.buffers[v4l2_buf_index].start[V4L2_FORMAT_Y_PLANE],(uint32_t *) drm.fbMem[drm.current_fb],vActiveIn, hActiveIn, v4l2_help.fmt.fmt.pix_mp.plane_fmt[V4L2_FORMAT_Y_PLANE].bytesperline, drm.create_dumb[drm.current_fb].pitch, coeff_gradient_v);
				break;
			case 9 :
				filter2d_cv((uint8_t *) v4l2_help.buffers[v4l2_buf_index].start[V4L2_FORMAT_Y_PLANE],(uint32_t *) drm.fbMem[drm.current_fb],vActiveIn, hActiveIn, v4l2_help.fmt.fmt.pix_mp.plane_fmt[V4L2_FORMAT_Y_PLANE].bytesperline, drm.create_dumb[drm.current_fb].pitch, coeff_sharpen);
				break;
			case 10 :
				filter2d_cv((uint8_t *) v4l2_help.buffers[v4l2_buf_index].start[V4L2_FORMAT_Y_PLANE],(uint32_t *) drm.fbMem[drm.current_fb],vActiveIn, hActiveIn, v4l2_help.fmt.fmt.pix_mp.plane_fmt[V4L2_FORMAT_Y_PLANE].bytesperline, drm.create_dumb[drm.current_fb].pitch, coeff_sobel_h);
				break;
			case 11 :
				filter2d_cv((uint8_t *) v4l2_help.buffers[v4l2_buf_index].start[V4L2_FORMAT_Y_PLANE],(uint32_t *) drm.fbMem[drm.current_fb],vActiveIn, hActiveIn, v4l2_help.fmt.fmt.pix_mp.plane_fmt[V4L2_FORMAT_Y_PLANE].bytesperline, drm.create_dumb[drm.current_fb].pitch, coeff_sobel_v);
				break;
			default :
				filter2d_cv((uint8_t *) v4l2_help.buffers[v4l2_buf_index].start[V4L2_FORMAT_Y_PLANE],(uint32_t *) drm.fbMem[drm.current_fb],vActiveIn, hActiveIn, v4l2_help.fmt.fmt.pix_mp.plane_fmt[V4L2_FORMAT_Y_PLANE].bytesperline, drm.create_dumb[drm.current_fb].pitch, coeff_identity);
			}
		}
		else
		{
			switch (sw)
			{
			case 0 :
				filter2d_xf((uint8_t *) v4l2_help.buffers[v4l2_buf_index].start[V4L2_FORMAT_Y_PLANE], (rgbpxl_t *)drm.fbMem[drm.current_fb], vActiveIn, hActiveIn, drm.create_dumb[drm.current_fb].pitch/DISPLAY_BPP, coeff_off);
				break;
			case 2 :
				filter2d_xf((uint8_t *) v4l2_help.buffers[v4l2_buf_index].start[V4L2_FORMAT_Y_PLANE], (rgbpxl_t *)drm.fbMem[drm.current_fb], vActiveIn, hActiveIn, drm.create_dumb[drm.current_fb].pitch/DISPLAY_BPP, coeff_blur);
				break;
			case 3 :
				filter2d_xf((uint8_t *) v4l2_help.buffers[v4l2_buf_index].start[V4L2_FORMAT_Y_PLANE], (rgbpxl_t *)drm.fbMem[drm.current_fb], vActiveIn, hActiveIn, drm.create_dumb[drm.current_fb].pitch/DISPLAY_BPP, coeff_edge);
				break;
			case 4 :
				filter2d_xf((uint8_t *) v4l2_help.buffers[v4l2_buf_index].start[V4L2_FORMAT_Y_PLANE], (rgbpxl_t *)drm.fbMem[drm.current_fb], vActiveIn, hActiveIn, drm.create_dumb[drm.current_fb].pitch/DISPLAY_BPP, coeff_edge_h);
				break;
			case 5 :
				filter2d_xf((uint8_t *) v4l2_help.buffers[v4l2_buf_index].start[V4L2_FORMAT_Y_PLANE], (rgbpxl_t *)drm.fbMem[drm.current_fb], vActiveIn, hActiveIn, drm.create_dumb[drm.current_fb].pitch/DISPLAY_BPP, coeff_edge_v);
				break;
			case 6 :
				filter2d_xf((uint8_t *) v4l2_help.buffers[v4l2_buf_index].start[V4L2_FORMAT_Y_PLANE], (rgbpxl_t *)drm.fbMem[drm.current_fb], vActiveIn, hActiveIn, drm.create_dumb[drm.current_fb].pitch/DISPLAY_BPP, coeff_emboss);
				break;
			case 7 :
				filter2d_xf((uint8_t *) v4l2_help.buffers[v4l2_buf_index].start[V4L2_FORMAT_Y_PLANE], (rgbpxl_t *)drm.fbMem[drm.current_fb], vActiveIn, hActiveIn, drm.create_dumb[drm.current_fb].pitch/DISPLAY_BPP, coeff_gradient_h);
				break;
			case 8 :
				filter2d_xf((uint8_t *) v4l2_help.buffers[v4l2_buf_index].start[V4L2_FORMAT_Y_PLANE], (rgbpxl_t *)drm.fbMem[drm.current_fb], vActiveIn, hActiveIn, drm.create_dumb[drm.current_fb].pitch/DISPLAY_BPP, coeff_gradient_v);
				break;
			case 9 :
				filter2d_xf((uint8_t *) v4l2_help.buffers[v4l2_buf_index].start[V4L2_FORMAT_Y_PLANE], (rgbpxl_t *)drm.fbMem[drm.current_fb], vActiveIn, hActiveIn, drm.create_dumb[drm.current_fb].pitch/DISPLAY_BPP, coeff_sharpen);
				break;
			case 10 :
				filter2d_xf((uint8_t *) v4l2_help.buffers[v4l2_buf_index].start[V4L2_FORMAT_Y_PLANE], (rgbpxl_t *)drm.fbMem[drm.current_fb], vActiveIn, hActiveIn, drm.create_dumb[drm.current_fb].pitch/DISPLAY_BPP, coeff_sobel_h);
				break;
			case 11 :
				filter2d_xf((uint8_t *) v4l2_help.buffers[v4l2_buf_index].start[V4L2_FORMAT_Y_PLANE], (rgbpxl_t *)drm.fbMem[drm.current_fb], vActiveIn, hActiveIn, drm.create_dumb[drm.current_fb].pitch/DISPLAY_BPP, coeff_sobel_v);
				break;
			default :
				filter2d_xf((uint8_t *) v4l2_help.buffers[v4l2_buf_index].start[V4L2_FORMAT_Y_PLANE], (rgbpxl_t *)drm.fbMem[drm.current_fb], vActiveIn, hActiveIn, drm.create_dumb[drm.current_fb].pitch/DISPLAY_BPP, coeff_identity);
			}
		}

		v4l2_helper_queue(&v4l2_help, v4l2_buf_index);

		i++;
		if (i == 30)
		{
			timeElapsed = (sds_clock_counter()- ticks);
			frameRate = ((double) sds_clock_frequency())/((double) timeElapsed) * 30.0;
			mvprintw(7,0,"Framerate = %3.3f     ", frameRate);
			wrefresh(mywin);
			ticks = sds_clock_counter();
			i = 0;
		}
		userInput = getch();
	}


	endwin();

	/*
	 * Unregister v4l2 buffers with SDSoC driver
	 */
    for (int i = 0; i < v4l2_help.num_buf; i++)
		for (int j = 0; j < v4l2_help.fmt.fmt.pix_mp.num_planes; j++)
			sds_unregister_dmabuf(v4l2_help.buffers[i].start[j],v4l2_help.buffers[i].dmabuf_fd[j]);

    /*
     * Free all v4l2 buffers and close all files
     */
    v4l2_helper_stop_cap(&v4l2_help);
    v4l2_helper_uninit(&v4l2_help);
    v4l2_helper_close(&v4l2_help);

    /*
     * Unregister and free DRM buffers and close DRM files
     */
    drmControlClose(&drm);

    /*
     * Close /dev/mem and gpio switch files
     */
	close(mem_fd);
	close(sw_fd);

	/*Reactivate terminal*/
	tty_fd = open("/dev/tty1", O_RDWR);
	ioctl(tty_fd,KDSETMODE,KD_TEXT);
	close(tty_fd);

    return 0;
}
