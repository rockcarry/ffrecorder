#!/bin/sh

set -e

gcc -Wall -static -Ilibfaac/include -Ilibx264/include utils.c ringbuf.c alawenc.c aacenc.c h264enc.c bufenc.c avimuxer.c mp4muxer.c recorder.c test.c -Llibfaac/lib -lfaac -Llibx264/lib -lx264 -lpthread -lm -o test
