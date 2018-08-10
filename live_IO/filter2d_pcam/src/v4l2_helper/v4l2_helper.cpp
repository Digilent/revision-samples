
    /*
     *  V4L2 video capture example
     *
     *  This program can be used and distributed without restrictions.
     *
     *      This program is provided with the V4L2 API
     * see https://linuxtv.org/docs.php for more information
     */

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <assert.h>

    #include <getopt.h>             /* getopt_long() */

    #include <fcntl.h>              /* low-level i/o */
    #include <unistd.h>
    #include <errno.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <sys/time.h>
    #include <sys/mman.h>
    #include <sys/ioctl.h>

    #include <linux/videodev2.h>
    #include <linux/fb.h>
    #include <linux/kd.h>

	#include "v4l2_helper/v4l2_helper.h"

    #define CLEAR(x) memset(&(x), 0, sizeof(x))
	#define SUCCESS 0

    static void errno_exit(const char *s)
    {
	    fprintf(stderr, "%s error %d, %s\\n", s, errno, strerror(errno));
	    exit(EXIT_FAILURE);
    }

    static int xioctl(int fh, int request, void *arg)
    {
	    int r;

	    do {
		    r = ioctl(fh, request, arg);
	    } while (-1 == r && EINTR == errno);

	    return r;
    }

    /*
     * Attempt to dequeue a buffer and return immediately.
     * Exits on failure.
     *
     * returns 0 if no buffer is ready
     * returns index of dequeued buffer otherwise
     *
     */
    unsigned int v4l2_helper_dequeue(v4l2_helper * control)
    {
	    struct v4l2_buffer buf;
	    struct v4l2_plane planes[VIDEO_MAX_PLANES] = { 0 };

		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.m.planes = planes;
		buf.length = control->fmt.fmt.pix_mp.num_planes;

		if (-1 == xioctl(control->video_fd, VIDIOC_DQBUF, &buf)) {
			switch (errno) {
			case EAGAIN:
				return 0;

			case EIO:
				/* Could ignore EIO, see spec. */

				/* fall through */

			default:
				errno_exit("VIDIOC_DQBUF");
			}
		}
		assert(buf.index < control->num_buf);
		return buf.index;
    }

    /*
     * Attempt to dequeue a buffer and block until one is ready.
     * Exit on failure or 2 second time out.
     *
     * Returns index of dequeued buffer
     */
    unsigned int v4l2_helper_readframe(v4l2_helper * control)
    {
    	unsigned int index = 0;
		for (;;) {
			fd_set fds;
			struct timeval tv;
			int r;

			FD_ZERO(&fds);
			FD_SET(control->video_fd, &fds);

			/* Timeout. */
			tv.tv_sec = 2;
			tv.tv_usec = 0;

			r = select(control->video_fd + 1, &fds, NULL, NULL, &tv);

			if (-1 == r) {
				if (EINTR == errno)
					continue;
				errno_exit("select");
			}

			if (0 == r) {
				fprintf(stderr, "select timeout\\n");
				exit(EXIT_FAILURE);
			}

			index = v4l2_helper_dequeue(control);
			if (index)
				break;
			/* EAGAIN - continue select loop. */
		}
		return index;
    }

    void v4l2_helper_stop_cap(v4l2_helper * control)
    {
	    enum v4l2_buf_type type;
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		if (-1 == xioctl(control->video_fd, VIDIOC_STREAMOFF, &type))
			errno_exit("VIDIOC_STREAMOFF");
    }

    int v4l2_helper_queue(v4l2_helper * control, unsigned int index)
	{
    	struct v4l2_buffer buf;

		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = index;
		buf.length = control->fmt.fmt.pix_mp.num_planes;
		buf.m.planes = control->buffers[index].planes;

		if (-1 == xioctl(control->video_fd, VIDIOC_QBUF, &buf))
			errno_exit("VIDIOC_QBUF");
		return SUCCESS;
	}

    int v4l2_helper_start_cap(v4l2_helper * control)
    {
	    unsigned int i;
	    enum v4l2_buf_type type;

		for (i = 0; i < control->num_buf; ++i) {
			v4l2_helper_queue(control, i);
		}
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		if (-1 == xioctl(control->video_fd, VIDIOC_STREAMON, &type))
			errno_exit("VIDIOC_STREAMON");

		return SUCCESS;
    }

    void v4l2_helper_uninit(v4l2_helper * control)
    {
	    unsigned int i, j;

		for (i = 0; i < control->num_buf; ++i)
			for (j = 0; j < control->fmt.fmt.pix_mp.num_planes; j++)
			{
				if (-1 == munmap(control->buffers[i].start[j], control->buffers[i].length[j]))
					fprintf(stderr, "ERROR: munmap on buffer %d, plane %d failed", i, j);
				close(control->buffers[i].dmabuf_fd[j]);
			}
    }

    int v4l2_helper_init(v4l2_helper * control, unsigned int num_buf)
    {
	    struct v4l2_capability cap;
	    struct v4l2_cropcap cropcap;
	    struct v4l2_crop crop;
	    struct v4l2_requestbuffers req;
	    unsigned int i, j;

	    if (num_buf > V4L2_HELPER_MAX_BUF) {
	    	fprintf(stderr, "ERROR: Requested %d V4L2 buffers, Max allowed=%d",
	    			num_buf, V4L2_HELPER_MAX_BUF);
	    	exit(EXIT_FAILURE);
	    }

	    if (-1 == xioctl(control->video_fd, VIDIOC_QUERYCAP, &cap)) {
		    if (EINVAL == errno) {
			    fprintf(stderr, "%s is no V4L2 device\\n",
			    		control->dev_name);
			    exit(EXIT_FAILURE);
		    } else {
			    errno_exit("VIDIOC_QUERYCAP");
		    }
	    }

	    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE)) {
		    fprintf(stderr, "%s is no m-plane video capture device. cap=%x\n",
		    		control->dev_name, cap.capabilities);
		    exit(EXIT_FAILURE);
	    }

		if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
			fprintf(stderr, "%s does not support streaming i/o\\n",
					control->dev_name);
			exit(EXIT_FAILURE);
		}


	    /* Select video input, video standard and tune here. */


	    CLEAR(cropcap);

	    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

	    if (0 == xioctl(control->video_fd, VIDIOC_CROPCAP, &cropcap)) {
		    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		    crop.c = cropcap.defrect; /* reset to default */

		    if (-1 == xioctl(control->video_fd, VIDIOC_S_CROP, &crop)) {
		    	printf("WARNING: crop ioctl failure:");
		    	switch (errno) {
			    case EINVAL:
			    	printf("EINVAL\n");
				    /* Cropping not supported. */
				    break;
			    default:
			    	printf("other=%d\n",errno);
				    /* Errors ignored. */
				    break;
			    }
		    }
	    } else {
	    	printf("WARNING: crop capability ioctl failure\n");
		    /* Errors ignored. */
	    }


	    CLEAR(control->fmt);
	    control->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		/* Preserve original settings as set by v4l2-ctl for example */
		if (-1 == xioctl(control->video_fd, VIDIOC_G_FMT, &control->fmt))
			errno_exit("VIDIOC_G_FMT");

//	    /* Buggy driver paranoia. */
//	    min = control->fmt.fmt.pix.width * 2;
//	    if (control->fmt.fmt.pix.bytesperline < min) {
//	    	control->fmt.fmt.pix.bytesperline = min;
//		    printf("WARNING: bytes Per Line override\n");
//	    }
//		min = control->fmt.fmt.pix.bytesperline * control->fmt.fmt.pix.height;
//	    if (control->fmt.fmt.pix.sizeimage < min) {
//	    	control->fmt.fmt.pix.sizeimage = min;
//		    printf("WARNING: image size override\n");
//	    }

	    control->num_buf = num_buf;

	    CLEAR(req);

	    req.count  = num_buf;
	    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	    req.memory = V4L2_MEMORY_MMAP;

	    if (-1 == xioctl(control->video_fd, VIDIOC_REQBUFS, &req)) {
		    if (EINVAL == errno) {
		    	printf("Video capturing or mmap-streaming is not supported\\n");
			    exit(EXIT_FAILURE);
		    } else {
			    errno_exit("VIDIOC_REQBUFS");
		    }
	    }

	    if (req.count != num_buf)
	    	printf("WARNING: Requested %d V4L2 buffers, only received %d",
	    			num_buf, req.count);
	    control->num_buf = req.count;

	    for (i = 0; i < control->num_buf; i++)
	    {
	    	struct v4l2_buffer buffer;

			memset(&buffer, 0, sizeof(buffer));
			buffer.type = req.type;
			buffer.memory = V4L2_MEMORY_MMAP;
			buffer.index = i;
			/* length in struct v4l2_buffer in multi-planar API stores the size
			 * of planes array. */
			buffer.length = control->fmt.fmt.pix_mp.num_planes;
			buffer.m.planes = control->buffers[i].planes;

			if (ioctl(control->video_fd, VIDIOC_QUERYBUF, &buffer) < 0) {
				perror("VIDIOC_QUERYBUF");
				exit(EXIT_FAILURE);
			}
			for (j = 0; j < control->fmt.fmt.pix_mp.num_planes; j++) {
				struct v4l2_exportbuffer expbuf;

				control->buffers[i].length[j] = buffer.m.planes[j].length;
				control->buffers[i].start[j] = mmap(NULL, buffer.m.planes[j].length,
						 PROT_READ | PROT_WRITE, /* recommended */
						 MAP_SHARED,             /* recommended */
						 control->video_fd, buffer.m.planes[j].m.mem_offset);

				if (MAP_FAILED == control->buffers[i].start[j]) {
					/* If you do not exit here you should unmap() and free()
					   the buffers and planes mapped so far. */
					perror("mmap");
					exit(EXIT_FAILURE);
				}

				memset(&expbuf, 0, sizeof(expbuf));
				expbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
				expbuf.index = i;
				expbuf.plane = j;
				if (ioctl(control->video_fd, VIDIOC_EXPBUF, &expbuf) == -1) {
					perror("VIDIOC_EXPBUF");
					exit(EXIT_FAILURE);
				}
				control->buffers[i].dmabuf_fd[j] = expbuf.fd;
			}
	    }
	    return SUCCESS;
    }

    void v4l2_helper_close(v4l2_helper *control)
    {
	    close(control->video_fd);

	    control->video_fd = -1;
    }

    int v4l2_helper_open(v4l2_helper *control, char * dev_name)
    {
	    struct stat st;

	    if (-1 == stat(dev_name, &st)) {
		    fprintf(stderr, "Cannot identify '%s': %d, %s\\n",
			     dev_name, errno, strerror(errno));
	    }

	    if (!S_ISCHR(st.st_mode)) {
		    fprintf(stderr, "%s is no devicen", dev_name);
	    }

	   control->dev_name = dev_name;
	   return control->video_fd = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

    }

