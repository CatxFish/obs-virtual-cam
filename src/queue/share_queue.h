#pragma once

#include <Windows.h>
#include "libavutil/pixfmt.h"
#include "libavutil/samplefmt.h"

#define AUDIO_SIZE 4096
#define MAPPING_NAMEV  "OBSVirtualVideo"
#define MAPPING_NAMEV2 "OBSVirtualVideo2"
#define MAPPING_NAMEV3 "OBSVirtualVideo3"
#define MAPPING_NAMEV4 "OBSVirtualVideo4"
#define MAPPING_NAMEA  "OBSVirtualAudio"

typedef signed char        int8_t;
typedef short              int16_t;
typedef int                int32_t;
typedef long long          int64_t;
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

enum {
	ModeVideo = 0,
	ModeVideo2 = 1,
	ModeVideo3 = 2,
	ModeVideo4 = 3,
	ModeAudio = 5,
};

enum {
	OutputStop = 0,
	OutputStart = 1,
	OutputReady = 2,
};

struct frame_header {
	uint64_t timestamp;
	uint32_t linesize[4];
	int frame_width;
	int frame_height;
};

struct queue_header {
	int state;
	int format;
	int queue_length;
	int write_index;
	int header_size;
	int element_size;
	int element_header_size;
	int delay_frame;
	int recommended_width;
	int recommended_height;
	int aspect_ratio_type;
	uint64_t last_ts;
	uint64_t frame_time;
};

struct share_queue {
	int mode =0 ;
	int index = -1;
	int operating_width;
	int operating_height;
	HANDLE hwnd =NULL;
	queue_header* header = nullptr;
};

inline char* get_mapping_name(int mode)
{
	switch (mode){
		case ModeVideo:    return MAPPING_NAMEV;
		case ModeVideo2:   return MAPPING_NAMEV2;
		case ModeVideo3:   return MAPPING_NAMEV3;
		case ModeVideo4:   return MAPPING_NAMEV4;
		case ModeAudio:    return MAPPING_NAMEA;
		default:           return NULL;
	}
}

inline frame_header* get_frame_header(queue_header* qhead, int index)
{
	int offset = qhead->header_size + (qhead->element_size) * index;
	uint8_t* buff = (uint8_t*)qhead + offset;
	return (frame_header*)buff;
}

inline int cal_video_buffer_size(int format, int width, int height)
{
	int frame_size = 0;
	switch (format) { 
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