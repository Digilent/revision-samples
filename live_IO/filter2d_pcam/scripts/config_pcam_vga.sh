#!/bin/bash

media-ctl -d /dev/media0 -V '"ov5640 2-003c":0 [fmt:UYVY/640x480@1/60 field:none]'
media-ctl -d /dev/media0 -V '"43c80000.mipi_csi2_rx_subsystem":0 [fmt:UYVY/640x480 field:none]'
v4l2-ctl -d /dev/video0 --set-fmt-video=width=640,height=480,pixelformat='NM16'
yavta -c1 -n3 -f NV16M -s 640x480 -F /dev/video0


