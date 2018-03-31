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
#include "video_uio/video_uio.h"
#include "rgbpxl.h"

int main(int argc, char **argv)
{
    uint32_t hActiveIn, vActiveIn;
    int sw_fd, tty_fd, mem_fd;;
    uint8_t *swMem;
    rgbpxl_t *fb_buf[NUM_INPUT_FB];
    uint32_t fb_addr[NUM_INPUT_FB] = {FB0_ADDR, FB1_ADDR, FB2_ADDR, FB3_ADDR};
    struct drm_cntrl drm = {0};

    /*
     * Initialize the Video pipeline and detect the frame dimensions of the source
     */
    if (video_uio_detect(UIO_GPIO_PATH, UIO_VTC_PATH, &hActiveIn, &vActiveIn) != VIDEO_UIO_SUCCESS)
    {
    	printf("Could not detect video source\n");
    	return -1;
    }

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
     * mmap input framebuffers
     */
	mem_fd = open("/dev/mem", O_RDWR);
    if(mem_fd == -1) {
	   printf("Could not open /dev/mem\n");
	   return -1;
    }
    for (int i = 0; i < NUM_INPUT_FB; i++)
    {
    	fb_buf[i] = (rgbpxl_t *) sds_mmap((void *) fb_addr[i], (drm.create_dumb[drm.current_fb].pitch*drm.create_dumb[drm.current_fb].height), NULL);
    }

	/*
     *Start VDMA
     */
    video_uio_start(UIO_DMA_PATH,FB0_ADDR,hActiveIn,vActiveIn,DISPLAY_BPP,drm.create_dumb[drm.current_fb].pitch);


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
	printw("Input Resolution: %dx%d\n",hActiveIn,vActiveIn);
	printw("Output Resolution: %dx%d\n",drm.current.hdisplay,drm.current.vdisplay);
	printw("FB Stride (bytes): %d\n",drm.create_dumb[drm.current_fb].pitch);
	printw("\n");
	printw("Press any key to exit...");
	wrefresh(mywin);

	unsigned long long ticks = sds_clock_counter();
	unsigned long long timeElapsed;
	int i = 0;
	double frameRate = 0;
	int userInput = ERR;
	while (userInput == ERR)
	{
		uint32_t sw = UioRead32(swMem, 0);
		uint32_t btn = UioRead32(swMem, 8);
		if (btn & 1)
		{
			switch (sw)
			{
			case 0 :
				filter2d_cv((uint32_t *) fb_buf[0],(uint32_t *) drm.fbMem[drm.current_fb],vActiveIn, hActiveIn, drm.create_dumb[drm.current_fb].pitch, coeff_off);
				break;
			case 2 :
				filter2d_cv((uint32_t *) fb_buf[0],(uint32_t *) drm.fbMem[drm.current_fb],vActiveIn, hActiveIn, drm.create_dumb[drm.current_fb].pitch, coeff_blur);
				break;
			case 3 :
				filter2d_cv((uint32_t *) fb_buf[0],(uint32_t *) drm.fbMem[drm.current_fb],vActiveIn, hActiveIn, drm.create_dumb[drm.current_fb].pitch, coeff_edge);
				break;
			case 4 :
				filter2d_cv((uint32_t *) fb_buf[0],(uint32_t *) drm.fbMem[drm.current_fb],vActiveIn, hActiveIn, drm.create_dumb[drm.current_fb].pitch, coeff_edge_h);
				break;
			case 5 :
				filter2d_cv((uint32_t *) fb_buf[0],(uint32_t *) drm.fbMem[drm.current_fb],vActiveIn, hActiveIn, drm.create_dumb[drm.current_fb].pitch, coeff_edge_v);
				break;
			case 6 :
				filter2d_cv((uint32_t *) fb_buf[0],(uint32_t *) drm.fbMem[drm.current_fb],vActiveIn, hActiveIn, drm.create_dumb[drm.current_fb].pitch, coeff_emboss);
				break;
			case 7 :
				filter2d_cv((uint32_t *) fb_buf[0],(uint32_t *) drm.fbMem[drm.current_fb],vActiveIn, hActiveIn, drm.create_dumb[drm.current_fb].pitch, coeff_gradient_h);
				break;
			case 8 :
				filter2d_cv((uint32_t *) fb_buf[0],(uint32_t *) drm.fbMem[drm.current_fb],vActiveIn, hActiveIn, drm.create_dumb[drm.current_fb].pitch, coeff_gradient_v);
				break;
			case 9 :
				filter2d_cv((uint32_t *) fb_buf[0],(uint32_t *) drm.fbMem[drm.current_fb],vActiveIn, hActiveIn, drm.create_dumb[drm.current_fb].pitch, coeff_sharpen);
				break;
			case 10 :
				filter2d_cv((uint32_t *) fb_buf[0],(uint32_t *) drm.fbMem[drm.current_fb],vActiveIn, hActiveIn, drm.create_dumb[drm.current_fb].pitch, coeff_sobel_h);
				break;
			case 11 :
				filter2d_cv((uint32_t *) fb_buf[0],(uint32_t *) drm.fbMem[drm.current_fb],vActiveIn, hActiveIn, drm.create_dumb[drm.current_fb].pitch, coeff_sobel_v);
				break;
			default :
				filter2d_cv((uint32_t *) fb_buf[0],(uint32_t *) drm.fbMem[drm.current_fb],vActiveIn, hActiveIn, drm.create_dumb[drm.current_fb].pitch, coeff_identity);
			}
		}
		else
		{
			switch (sw)
			{
			case 0 :
				filter2d_xf(fb_buf[0], (rgbpxl_t *)drm.fbMem[drm.current_fb], vActiveIn, hActiveIn, drm.create_dumb[drm.current_fb].pitch/DISPLAY_BPP, coeff_off);
				break;
			case 2 :
				filter2d_xf(fb_buf[0], (rgbpxl_t *)drm.fbMem[drm.current_fb], vActiveIn, hActiveIn, drm.create_dumb[drm.current_fb].pitch/DISPLAY_BPP, coeff_blur);
				break;
			case 3 :
				filter2d_xf(fb_buf[0], (rgbpxl_t *)drm.fbMem[drm.current_fb], vActiveIn, hActiveIn, drm.create_dumb[drm.current_fb].pitch/DISPLAY_BPP, coeff_edge);
				break;
			case 4 :
				filter2d_xf(fb_buf[0], (rgbpxl_t *)drm.fbMem[drm.current_fb], vActiveIn, hActiveIn, drm.create_dumb[drm.current_fb].pitch/DISPLAY_BPP, coeff_edge_h);
				break;
			case 5 :
				filter2d_xf(fb_buf[0], (rgbpxl_t *)drm.fbMem[drm.current_fb], vActiveIn, hActiveIn, drm.create_dumb[drm.current_fb].pitch/DISPLAY_BPP, coeff_edge_v);
				break;
			case 6 :
				filter2d_xf(fb_buf[0], (rgbpxl_t *)drm.fbMem[drm.current_fb], vActiveIn, hActiveIn, drm.create_dumb[drm.current_fb].pitch/DISPLAY_BPP, coeff_emboss);
				break;
			case 7 :
				filter2d_xf(fb_buf[0], (rgbpxl_t *)drm.fbMem[drm.current_fb], vActiveIn, hActiveIn, drm.create_dumb[drm.current_fb].pitch/DISPLAY_BPP, coeff_gradient_h);
				break;
			case 8 :
				filter2d_xf(fb_buf[0], (rgbpxl_t *)drm.fbMem[drm.current_fb], vActiveIn, hActiveIn, drm.create_dumb[drm.current_fb].pitch/DISPLAY_BPP, coeff_gradient_v);
				break;
			case 9 :
				filter2d_xf(fb_buf[0], (rgbpxl_t *)drm.fbMem[drm.current_fb], vActiveIn, hActiveIn, drm.create_dumb[drm.current_fb].pitch/DISPLAY_BPP, coeff_sharpen);
				break;
			case 10 :
				filter2d_xf(fb_buf[0], (rgbpxl_t *)drm.fbMem[drm.current_fb], vActiveIn, hActiveIn, drm.create_dumb[drm.current_fb].pitch/DISPLAY_BPP, coeff_sobel_h);
				break;
			case 11 :
				filter2d_xf(fb_buf[0], (rgbpxl_t *)drm.fbMem[drm.current_fb], vActiveIn, hActiveIn, drm.create_dumb[drm.current_fb].pitch/DISPLAY_BPP, coeff_sobel_v);
				break;
			default :
				filter2d_xf(fb_buf[0], (rgbpxl_t *)drm.fbMem[drm.current_fb], vActiveIn, hActiveIn, drm.create_dumb[drm.current_fb].pitch/DISPLAY_BPP, coeff_identity);
			}
		}
		i++;
		if (i == 30)
		{
			timeElapsed = (sds_clock_counter()- ticks);
			frameRate = ((double) sds_clock_frequency())/((double) timeElapsed) * 30.0;
			mvprintw(4,0,"Framerate = %3.3f     ", frameRate);
			wrefresh(mywin);
			ticks = sds_clock_counter();
			i = 0;
		}
		userInput = getch();
	}


	endwin();

	close(mem_fd);
	close(sw_fd);
	/*Reactivate terminal*/
	tty_fd = open("/dev/tty1", O_RDWR);
	ioctl(tty_fd,KDSETMODE,KD_TEXT);
	close(tty_fd);

    return 0;
}

void exit_clean(struct drm_cntrl * drm)
{
	return;
}
