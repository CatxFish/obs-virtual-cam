#pragma once

#include <Windows.h>
#include "libavutil/pixfmt.h"
#include "libavutil/samplefmt.h"

#define VIDEO_SIZE 1920*1080*4
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

