#include "share_queue.h"
bool shared_queue_create(share_queue* q, int mode, int format,
	int frame_width, int frame_height, uint64_t frame_time, int qlength);
void shared_queue_write_close(share_queue* q);
bool shared_queue_push_video(share_queue* q, uint32_t* linesize,
	uint32_t width, uint32_t height, uint8_t** data, uint64_t timestamp);
bool shared_queue_push_audio(share_queue* q, uint32_t size,
	uint8_t* src, uint64_t timestamp, uint64_t video_ts);
bool shared_queue_check(int mode);
bool shared_queue_set_delay(share_queue* q, int delay_video_frame);
bool shared_queue_set_keep_ratio(share_queue* q, bool keep_ratio);
bool shared_queue_set_recommended_format(share_queue* q, int width, 
	int height);