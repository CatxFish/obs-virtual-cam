#pragma once

#include <Windows.h>
#include "libavutil/pixfmt.h"
#include "libavutil/samplefmt.h"

#define AUDIO_SIZE 4096
#define MAPPING_NAMEV "OBSVirtualVideo"
#define MAPPING_NAMEA "OBSVirtualAudio"

typedef signed char        int8_t;
typedef short              int16_t;
typedef int                int32_t;
typedef long long          int64_t;
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

enum 
{
	ModeVideo = 0,
	ModeAudio =1,
};

enum
{
	OutputStop =0,
	OutputStart=1,
	OutputReady = 2,
};

struct frame_header
{
	uint64_t timestamp;
	uint32_t linesize[4];
	int frame_width;
	int frame_height;
};

struct queue_header
{
	int state;
	int format;
	int queue_length;
	int write_index;
	int header_size;
	int element_size;
	int element_header_size;
	int delay_frame;
	int canvas_width;
	int canvas_height;
	int aspect_ratio_type;
	uint64_t last_ts;
	int64_t frame_time;
};

struct share_queue
{
	int mode =0 ;
	int index = -1;
	int operating_width;
	int operating_height;
	HANDLE hwnd =NULL;
	queue_header* header = nullptr;
};

inline frame_header* get_frame_header(queue_header* qhead, int index)
{
	int offset = qhead->header_size + (qhead->element_size)*index;
	uint8_t* buff = (uint8_t*)qhead + offset;
	return (frame_header*)buff;
}

inline int cal_video_buffer_size(int format,int width, int height)
{
	int frame_size = 0;
	switch (format){ 
		case AV_PIX_FMT_YUV420P:
		case AV_PIX_FMT_NV12:
			frame_size = width * height * 3 / 2;
			break;

		case AV_PIX_FMT_GRAY8:
			frame_size = width * height;
			break;

		case AV_PIX_FMT_YUYV422:
		case AV_PIX_FMT_UYVY422:
			frame_size = width * height *2;
			break;

		case AV_PIX_FMT_RGBA:
		case AV_PIX_FMT_BGRA:
			frame_size = width * height * 4;
			break;

		case AV_PIX_FMT_YUV444P:
			frame_size = width * height * 3;
			break;	
	}

	return frame_size;
}