#ifndef SRC_V4L2_HELPER_H_
#define SRC_V4L2_HELPER_H_

#define V4L2_HELPER_MAX_BUF 32

#include <linux/videodev2.h>

typedef struct v4l2_helper_buf{
	void *start[VIDEO_MAX_PLANES];
	size_t length[VIDEO_MAX_PLANES];
	int dmabuf_fd[VIDEO_MAX_PLANES];
	v4l2_plane planes[VIDEO_MAX_PLANES];
} v4l2_helper_buf;

typedef struct v4l2_helper{
    int video_fd;
    char *dev_name;
    struct v4l2_format fmt;
    int num_buf;
    v4l2_helper_buf buffers[V4L2_HELPER_MAX_BUF];

} v4l2_helper;

unsigned int v4l2_helper_dequeue(v4l2_helper * control);
unsigned int v4l2_helper_readframe(v4l2_helper * control);

int v4l2_helper_queue(v4l2_helper * control, unsigned int index);

void v4l2_helper_stop_cap(v4l2_helper * control);
int v4l2_helper_start_cap(v4l2_helper * control);

void v4l2_helper_uninit(v4l2_helper * control);
int v4l2_helper_init(v4l2_helper * control, unsigned int num_buf);

void v4l2_helper_close(v4l2_helper *control);
int v4l2_helper_open(v4l2_helper *control, char * dev_name);


#endif
