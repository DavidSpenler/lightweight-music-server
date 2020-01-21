#!/bin/bash

#ffmpeg -i "$1" -c:v copy -c:a aac -start_number 0 -f hls -segment_time 2 -hls_time 2 -hls_list_size 0 -hls_playlist_type vod $2$3_.m3u8

ffmpeg -i "$1" -c:a aac -vn -hls_list_size 0 -hls_time 10 -hls_segment_filename $2$3_%03d.ts $2$3.m3u8
