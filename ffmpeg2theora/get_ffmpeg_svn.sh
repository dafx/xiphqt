#!/bin/sh

#optional, if you have those libs installed:
#apt-get install libamrnb-dev libamrwb-dev
#extra=" --enable-libamr_nb --enable-libamr_wb --enable-nonfree"

#apt-get install liba52-dev libfaad-dev libgsm1-dev
#extra=" --enable-libfaad --enable-libgsm"

common="--enable-gpl --enable-swscale --enable-postproc --disable-muxers --disable-encoders --enable-libvorbis"

#linux
options="$common --enable-pthreads $extra"

#mingw32
uname | grep MINGW && options="$common --enable-memalign-hack --enable-mingw32 --extra-cflags=-I/usr/local/include --extra-ldflags=-L/usr/local/lib $extra"

# load FFMPEG specific properties
#. ./ffmpegrev

#Get ffmpeg from svn
#svn -r $FFMPEG_REVISION co $FFMPEG_SVN $FFMPEG_CO_DIR
#svn update -r $FFMPEG_EXTERNALS_REVISION $FFMPEG_CO_DIR/libswscale

#get latest version from 0.5 branch
svn co svn://svn.ffmpeg.org/ffmpeg/branches/0.5 ffmpeg

#configure and build ffmpeg
cd ffmpeg && ./configure $options && make

