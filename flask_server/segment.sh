#!/bin/bash

ffmpeg -i "$1" -c:v copy -c:a aac -start_number 0 -f hls -hls_time 10 -hls_list_size 0 -hls_playlist_type vod $2$3_.m3u8
