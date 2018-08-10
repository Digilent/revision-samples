#!/bin/bash

media-ctl -d /dev/media0 -V '"ov5640 2-003c":0 [fmt:UYVY/1920x1080@1/15 field:none]'
media-ctl -d /dev/media0 -V '"43c80000.mipi_csi2_rx_subsystem":0 [fmt:UYVY/1920x1080 field:none]'
v4l2-ctl -d /dev/video0 --set-fmt-video=width=1920,height=1080,pixelformat='NM16'
yavta -c1 -n3 -f NV16M -s 1920x1080 -F /dev/video0

