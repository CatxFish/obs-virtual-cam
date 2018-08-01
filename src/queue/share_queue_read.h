#pragma once 
extern "C"
{
#include "libswscale/swscale.h" 
};
#include "share_queue.h"

struct dst_scale_context
{
	struct SwsContext *convert_ctx = nullptr;
	int dst_format = 0;
	int dst_width = 0;
	int dst_height = 0;
	int dst_offset = 0;
	int aspect_ratio_type = 0;
	int dst_linesize[4];	
};

bool shared_queue_open(share_queue* q, int mode);
void shared_queue_read_close(share_queue* q, dst_scale_context* scale_info);
bool share_queue_init_index(share_queue* q);
bool shared_queue_get_video_format(int mode, int* format, uint32_t* width,
	uint32_t* height, uint64_t* avgtime);
bool shared_queue_get_video(share_queue* q, dst_scale_context* scale_info,
	uint8_t* dst, uint64_t* timestamp);
bool shared_queue_get_audio(share_queue* q, uint8_t* dst,
	uint32_t max_size, uint64_t* timestamp);

