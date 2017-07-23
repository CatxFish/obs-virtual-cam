#include "share_queue.h"
#include "libavutil/pixfmt.h"


bool shared_queue_create(share_queue* q, int mode, int format, 
	int frame_width,int frame_height,int64_t frame_time,int qlength)
{
	if (!q)
		return false;

	if (!shared_queue_check(mode))
		return false;

	int size = 0;

	if (mode == ModeVideo){
		size = sizeof(queue_header) + (sizeof(frame_header) + VIDEO_SIZE)
			* qlength;
		q->hwnd = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 
			0, size, MAPPING_NAMEV);
	}
	else{
		size = sizeof(queue_header) + (sizeof(frame_header) + AUDIO_SIZE)
			* qlength;
		q->hwnd = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
			0, size, MAPPING_NAMEA);
	}

	if (q->hwnd)
		q->header = (queue_header*)MapViewOfFile(q->hwnd, FILE_MAP_ALL_ACCESS, 0, 0, size);

	queue_header* q_head = q->header;

	if (q_head){

		q_head->header_size = sizeof(queue_header);
		q_head->element_header_size = sizeof(frame_header);

		if (mode == ModeVideo)
			q_head->element_size = sizeof(frame_header) + VIDEO_SIZE;
		else
			q_head->element_size = sizeof(frame_header) + AUDIO_SIZE;
		
		q_head->format = format;
		q_head->queue_length = qlength;
		q_head->write_index = 0;
		q_head->frame_width = frame_width;
		q_head->frame_height = frame_height;
		q_head->state = OutputStart;
		q_head->frame_time = frame_time;
		q_head->delay_frame = 5;
		q->mode = mode;	
		q->index = 0;
		
	}

	return (q->hwnd != NULL && q->header != NULL);
}

bool shared_queue_open(share_queue* q, int mode)
{
	if (!q)
		return false;

	if (mode == ModeVideo)
		q->hwnd = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, MAPPING_NAMEV);
	else if (mode == ModeAudio)
		q->hwnd = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, MAPPING_NAMEA);
	else
		return false;

	if (q->hwnd){
		q->header = (queue_header*)MapViewOfFile(q->hwnd, 
			FILE_MAP_ALL_ACCESS,0, 0, 0);
	}
	else
		return false;
	
	if (q->header == nullptr){
		CloseHandle(q->hwnd);
		q->hwnd = nullptr;
		return false;
	}

	q->mode = mode;
	
	return true;
}

bool shared_queue_check(int mode)
{
	HANDLE hwnd =NULL;

	if (mode == ModeVideo)
		hwnd = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, MAPPING_NAMEV);
	else if (mode == ModeAudio)
		hwnd = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, MAPPING_NAMEA);
	else
		return false;

	if (hwnd){
		CloseHandle(hwnd);
		return false;
	}
	else
		return true;
}

void shared_queue_close(share_queue* q)
{
	if (q && q->header){
		q->header->state = OutputStop;
		UnmapViewOfFile(q->header);
		CloseHandle(q->hwnd);
		q->header = nullptr;
		q->hwnd = NULL;
		q->index = -1;
	}
}

bool shared_queue_set_delay(share_queue* q,int delay_video_frame)
{
	if (!q || !q->header)
		return false;

	q->header->delay_frame = delay_video_frame;
	return true;
}

bool share_queue_init_index(share_queue* q)
{
	if (!q || !q->header)
		return false;


	if (q->mode == ModeVideo){ 
		q->index = q->header->write_index - q->header->delay_frame;
		if (q->index < 0)
			q->index += q->header->queue_length;

	}else if (q->mode == ModeAudio){
		int write_index = q->header->write_index;
		int delay_frame = q->header->delay_frame;
		int64_t frame_time = q->header->frame_time;
		int64_t last_ts = q->header->last_ts;
		uint64_t start_ts = last_ts - delay_frame*frame_time;

		int index = write_index;
		uint64_t frame_ts = 0;

		do{
			index--;
			if (index < 0)
				index += q->header->queue_length;

			int offset = q->header->header_size +
				(q->header->element_size)*index;
			uint8_t* buff = (uint8_t*)q->header + offset;
			frame_ts = ((frame_header*)buff)->timestamp;

		} while (frame_ts > start_ts && index != write_index);

		if (index == write_index){
			q->index = write_index-q->header->queue_length/2;
			if (q->index < 0)
				q->index += q->header->queue_length;
		}
		else
			q->index = index;
	}

	return true;
}

bool shared_queue_get_video_format(share_queue* q,int* format ,int* width, 
	int* height, int64_t* avgtime)
{
	if (!q || !q->header)
		return false;

	*format = q->header->format;
	*width = q->header->frame_width;
	*height = q->header->frame_height;
	*avgtime = (q->header->frame_time)/100;
	return true;
}

bool shared_queue_get_video(share_queue* q, uint8_t** data, 
	uint32_t*linesize, uint64_t* timestamp)
{
	if (!q || !q->header)
		return false;

	if (q->index == q->header->write_index)
		return false;

	if (q->index < 0)
		share_queue_init_index(q);

	int offset = q->header->header_size +
		(q->header->element_size)*q->index;

	uint8_t* buff = (uint8_t*)q->header + offset;
	frame_header* head = (frame_header*)buff;
	buff += q->header->element_header_size;
	int height = q->header->frame_height;
	int planes = 0;

	switch (q->header->format) {
	case AV_PIX_FMT_NONE:
		return false;

	case AV_PIX_FMT_YUV420P:
		planes = 3;
		data[0] = buff;
		data[1] = data[0] + head->linesize[0] * height;
		data[2] = data[1] + head->linesize[1] * height / 2;
		break;

	case AV_PIX_FMT_NV12:
		planes = 2;
		data[0] = buff;
		data[1] = data[0] + head->linesize[0] * height;
		break;

	case AV_PIX_FMT_GRAY8:
	case AV_PIX_FMT_YUYV422:
	case AV_PIX_FMT_UYVY422:
	case AV_PIX_FMT_RGBA:
	case AV_PIX_FMT_BGRA:
		planes = 1;
		data[0] = buff;
		break;

	case AV_PIX_FMT_YUV444P:
		planes = 3;
		data[0] = buff;
		data[1] = data[0] + head->linesize[0] * height;
		data[2] = data[1] + head->linesize[1] * height;
		break;
	}

	for (int i = 0; i < planes; i++)
		linesize[i]=head->linesize[i] ;

	*timestamp = head->timestamp;

	q->index++;

	if (q->index >= q->header->queue_length) 
		q->index = 0; 

	return true;
}
bool shared_queue_push_video(share_queue* q, uint32_t* linesize,
	uint32_t height,uint8_t** src,uint64_t timestamp)
{
	if (!q || !q->header)
		return false;

	int offset = q->header->header_size +
		(q->header->element_size)*q->index;

	uint8_t* buff = (uint8_t*)q->header + offset;
	frame_header* head = (frame_header*)buff;
	uint8_t* data = (uint8_t*)buff + q->header->element_header_size;
	int planes = 0;

	switch (q->header->format) {
	case AV_PIX_FMT_NONE:
		return false;

	case AV_PIX_FMT_YUV420P:
		planes = 3;
		memcpy(data, src[0], linesize[0] * height);
		data += linesize[0] * height;
		memcpy(data, src[1], linesize[1] * height / 2);
		data += linesize[1] * height / 2;
		memcpy(data, src[2], linesize[2] * height / 2);	
		break;

	case AV_PIX_FMT_NV12:
		planes = 2;
		memcpy(data, src[0], linesize[0] * height);
		data += linesize[0] * height;
		memcpy(data, src[1], linesize[1] * height / 2);
		break;

	case AV_PIX_FMT_GRAY8:
	case AV_PIX_FMT_YUYV422:
	case AV_PIX_FMT_UYVY422:
	case AV_PIX_FMT_RGBA:
	case AV_PIX_FMT_BGRA:
		planes = 1;
		memcpy(data, src[0], linesize[0] * height);
		break;

	case AV_PIX_FMT_YUV444P:
		planes = 3;
		memcpy(data, src[0], linesize[0] * height);
		data += linesize[0] * height;
		memcpy(data, src[1], linesize[1] * height);
		data += linesize[1] * height;
		memcpy(data, src[2], linesize[2] * height);
		break;
	}

	for (int i = 0; i < planes;i++)
		head->linesize[i] = linesize[i];

	head->timestamp = timestamp;

	q->header->write_index = q->index;

	q->index++;

	if (q->index >= q->header->queue_length){
		q->header->state = OutputReady;
		q->index = 0;
	}

	return true;
}

bool shared_queue_get_audio(share_queue* q, uint8_t** data,
	uint32_t* size, uint64_t* timestamp)
{
	if (!q || !q->header)
		return false;

	if (q->index == q->header->write_index)
		return false;

	if (q->index < 0)
		share_queue_init_index(q);

	int offset = q->header->header_size +
		(q->header->element_size)*q->index;

	uint8_t* buff = (uint8_t*)q->header + offset;
	frame_header* head = (frame_header*)buff;
	buff += q->header->element_header_size;

	*data = buff;
	*size = head->linesize[0];
	*timestamp = head->timestamp;

	q->index++;

	if (q->index >= q->header->queue_length)
		q->index = 0;

	return true;
}

bool shared_queue_push_audio(share_queue* q, uint32_t size,
	uint8_t* src, uint64_t timestamp,uint64_t video_ts)
{
	if (!q || !q->header)
		return false;

	int offset = q->header->header_size +
		(q->header->element_size)*q->index;

	uint8_t* buff = (uint8_t*)q->header + offset;
	frame_header* head = (frame_header*)buff;
	uint8_t* data = (uint8_t*)buff + q->header->element_header_size;
	memcpy(data, src, size);
	head->linesize[0] = size;
	head->timestamp = timestamp;

	q->header->last_ts = video_ts;
	q->header->write_index = q->index;
	q->index++;

	if (q->index >= q->header->queue_length){
		q->index = 0;
		q->header->state = OutputReady;
	}

	return true;
}