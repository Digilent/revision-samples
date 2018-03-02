#include "video_uio/video_uio.h"
#include "uio/uio_ll.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>

int video_uio_detect(char const *gpioUioPath, char const *vtcUioPath, uint32_t *hDetect, uint32_t *vDetect)
{
	int gpio_fd, vtc_fd;
	uint8_t *gpioMem, *vtcMem;
	
    /*
     *uio initialization
     */
	gpio_fd = open(gpioUioPath, O_RDWR);
	vtc_fd = open(vtcUioPath, O_RDWR);

	gpioMem = (uint8_t *) mmap(0, UIO_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, gpio_fd, (off_t)0);
	vtcMem = (uint8_t *) mmap(0, UIO_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, vtc_fd, (off_t)0);

    /*
     *Check if HDMI input clock locked
     */
	UioWrite32(gpioMem, 0, 1); //
	UioWrite32(gpioMem, 4, 0); //set HPD as output and assert it
	UioWrite32(gpioMem, 12, 1); //set clock locked as input


	int isLocked = 0;
	for(int i = 0; i < INPUT_DETECT_TIMEOUT && !isLocked; i++)
	{
		sleep(1);
		isLocked = UioRead32(gpioMem, 8);
	}
	if (!isLocked)
	{
		close(gpio_fd);
		close(vtc_fd);
		fprintf(stderr, "HDMI input clock not detected, attach HDMI source");
		return VIDEO_UIO_FAILURE;
	}

    /*
     *Enable VTC and wait for it to lock
     */
	UioWrite32(vtcMem, 0, 8); //Enable detector
	isLocked = 0;
	for(int i = 0; i < INPUT_DETECT_TIMEOUT && !isLocked; i++)
	{
		sleep(1);
		isLocked = UioRead32(vtcMem, 0x24) & 1;
	}
	if (!isLocked) //check if locked
	{
		close(gpio_fd);
		close(vtc_fd);
		fprintf(stderr, "VTC not locked, attach a stable HDMI source");
		return VIDEO_UIO_FAILURE;
	}
	*hDetect = UioRead32(vtcMem,0x20);
	*vDetect = (*hDetect & 0x1FFF0000) >> 16;
	*hDetect = *hDetect & 0x1FFF;
	close(gpio_fd);
	close(vtc_fd);
	return VIDEO_UIO_SUCCESS;
}

void video_uio_start(char const *uioDmaPath, uint32_t fb_addr, uint32_t hActiveIn, uint32_t vActiveIn, uint32_t bytesPerPxl, uint32_t stride)
{
	int dma_fd;
	uint8_t *dmaMem;

	dma_fd = open(uioDmaPath, O_RDWR);
	dmaMem = (uint8_t *) mmap(0, UIO_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dma_fd, (off_t)0);

	/*
     *Start VDMA
     */
	UioWrite32(dmaMem, 0x30, 3); //Enable S2MM channel with Circular park
	UioWrite32(dmaMem, 0xAC, fb_addr); //Set the frame address to physical frame of display
	UioWrite32(dmaMem, 0x28, 0); //Set Park frame to frame 0
	UioWrite32(dmaMem, 0xA4, hActiveIn * bytesPerPxl); //Set horizontal active size in bytes
	UioWrite32(dmaMem, 0xA8, 0x1000000 | stride); //Set stride and preserve frame delay
	UioWrite32(dmaMem, 0xA0, vActiveIn); //Set the vertical active size, starting a transfer
	close(dma_fd);
}

