#pragma once

#include <Windows.h>

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
	int frame_width;
	int frame_height;
	int delay_frame;
	uint64_t last_ts;
	int64_t frame_time;
};

struct share_queue
{
	int mode =0 ;
	int index = -1;
	HANDLE hwnd =NULL;
	queue_header* header = nullptr;
};

bool shared_queue_create(share_queue* q, int mode, int format,
	int frame_width, int frame_height, int64_t frame_time, int qlength);
bool shared_queue_open(share_queue* q, int mode);
void shared_queue_close(share_queue* q);
bool shared_queue_check(int mode);
bool shared_queue_set_delay(share_queue* q, int delay_video_frame);
bool share_queue_init_index(share_queue* q);
bool shared_queue_get_video_format(share_queue* q, int* format, int* width,
	int* height, int64_t* avgtime);
bool shared_queue_get_video(share_queue* q, uint8_t** dst_ptr,
	uint32_t*linesize, uint64_t* timestamp);
bool shared_queue_push_video(share_queue* q, uint32_t* linesize,
	uint32_t height, uint8_t** src, uint64_t timestamp);
bool shared_queue_get_audio(share_queue* q, uint8_t** data,
	uint32_t* size, uint64_t* timestamp);
bool shared_queue_push_audio(share_queue* q, uint32_t size,
	uint8_t* src, uint64_t timestamp, uint64_t video_ts);

