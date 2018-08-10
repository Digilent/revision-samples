#!/bin/bash

media-ctl -d /dev/media0 -V '"ov5640 2-003c":0 [fmt:UYVY/1280x720@1/30 field:none]'
media-ctl -d /dev/media0 -V '"43c80000.mipi_csi2_rx_subsystem":0 [fmt:UYVY/1280x720 field:none]'
v4l2-ctl -d /dev/video0 --set-fmt-video=width=1280,height=720,pixelformat='NM16'
yavta -c1 -n3 -f NV16M -s 1280x720 -F /dev/video0

