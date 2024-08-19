#!/bin/bash

path=/home/i6sp/librtsp/build

processNum=`ps -ef | grep $path/V4l2Demo | grep -v grep | wc -l`

if [ $processNum -eq 0 ]; then
	echo "rtsp start"
	$path/V4l2Demo &
else
	echo "rtsp running"
fi

