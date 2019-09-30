#include "share_queue_write.h"

bool shared_queue_create(share_queue* q, int mode, int format,
	int width, int height, uint64_t frame_time, int qlength)
{
	if (!q)
		return false;

	if (!shared_queue_check(mode))
		return false;

	int frame_size = 0;
	int buffer_size = 0;
	const char* name = get_mapping_name(mode);

	if (mode < ModeAudio) {
		frame_size = cal_video_buffer_size(format, width, height);
		buffer_size = sizeof(queue_header) + (sizeof(frame_header) 
			+ frame_size) * qlength;
		q->hwnd = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, 
			PAGE_READWRITE, 0, buffer_size, name);
	} else {
		frame_size = AUDIO_SIZE;
		buffer_size = sizeof(queue_header) + (sizeof(frame_header) + 
			frame_size) * qlength;
		q->hwnd = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, 
			PAGE_READWRITE, 0, buffer_size, name);
	}

	if (q->hwnd) {
		q->header = (queue_header*)MapViewOfFile(q->hwnd, FILE_MAP_ALL_ACCESS, 
			0, 0, buffer_size);
	}
	queue_header* q_head = q->header;

	if (q_head) {
		q_head->header_size = sizeof(queue_header);
		q_head->element_header_size = sizeof(frame_header);
		q_head->element_size = sizeof(frame_header) + frame_size;
		q_head->format = format;
		q_head->queue_length = qlength;
		q_head->write_index = 0;
		q_head->state = OutputStart;
		q_head->frame_time = frame_time;
		q_head->delay_frame = 5;
		q_head->recommended_width = width;
		q_head->recommended_height = height;
		q->mode = mode;
		q->index = 0;
	}

	return (q->hwnd != NULL && q->header != NULL);
}

void shared_queue_write_close(share_queue* q)
{
	if (q && q->header) {
		q->header->state = OutputStop;
		UnmapViewOfFile(q->header);
		CloseHandle(q->hwnd);
		q->header = nullptr;
		q->hwnd = NULL;
		q->index = -1;
	}
}

bool shared_queue_push_video(share_queue* q, uint32_t* linesize, 
	uint32_t width, uint32_t height, uint8_t** data, uint64_t timestamp)
{
	if (!q || !q->header)
		return false;

	frame_header* frame = get_frame_header(q->header, q->index);
	uint8_t* dst = (uint8_t*)frame + q->header->element_header_size;
	int planes = 0;
	
	switch (q->header->format) {
	case AV_PIX_FMT_NONE:
		return false;

	case AV_PIX_FMT_YUV420P:
		planes = 3;
		memcpy(dst, data[0], linesize[0] * height);
		dst += linesize[0] * height;
		memcpy(dst, data[1], linesize[1] * height / 2);
		dst += linesize[1] * height / 2;
		memcpy(dst, data[2], linesize[2] * height / 2);
		break;

	case AV_PIX_FMT_NV12:
		planes = 2;
		memcpy(dst, data[0], linesize[0] * height);
		dst += linesize[0] * height;
		memcpy(dst, data[1], linesize[1] * height / 2);
		break;

	case AV_PIX_FMT_GRAY8:
	case AV_PIX_FMT_YUYV422:
	case AV_PIX_FMT_UYVY422:
	case AV_PIX_FMT_RGBA:
	case AV_PIX_FMT_BGRA:
		planes = 1;
		memcpy(dst, data[0], linesize[0] * height);
		break;

	case AV_PIX_FMT_YUV444P:
		planes = 3;
		memcpy(dst, data[0], linesize[0] * height);
		dst += linesize[0] * height;
		memcpy(dst, data[1], linesize[1] * height);
		dst += linesize[1] * height;
		memcpy(dst, data[2], linesize[2] * height);
		break;
	}

	for (int i = 0; i < planes; i++)
		frame->linesize[i] = linesize[i];

	frame->timestamp = timestamp;
	frame->frame_width = width;
	frame->frame_height = height;
	q->header->write_index = q->index;

	q->index++;

	if (q->index >= q->header->queue_length) {
		q->header->state = OutputReady;
		q->index = 0;
	}

	return true;
}

bool shared_queue_push_audio(share_queue* q, uint32_t size,
	uint8_t* src, uint64_t timestamp, uint64_t video_ts)
{
	if (!q || !q->header)
		return false;

	int offset = q->header->header_size +
		(q->header->element_size) * q->index;

	q->header->write_index = q->index;
	q->header->last_ts = video_ts;

	uint8_t* buff = (uint8_t*)q->header + offset;
	frame_header* head = (frame_header*)buff;
	uint8_t* data = (uint8_t*)buff + q->header->element_header_size;
	memcpy(data, src, size);
	head->linesize[0] = size;
	head->timestamp = timestamp;

	q->index++;

	if (q->index >= q->header->queue_length) {
		q->index = 0;
		q->header->state = OutputReady;
	}

	return true;
}

bool shared_queue_check(int mode)
{
	HANDLE hwnd = NULL;
	const char *name = get_mapping_name(mode);
	
	hwnd = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, name);

	if (hwnd) {
		CloseHandle(hwnd);
		return false;
	} else
		return true;
}

bool shared_queue_set_delay(share_queue* q, int delay_video_frame)
{
	if (!q || !q->header)
		return false;

	q->header->delay_frame = delay_video_frame;
	return true;
}

bool shared_queue_set_keep_ratio(share_queue* q, bool keep_ratio)
{
	if (!q || !q->header)
		return false;

	if (keep_ratio)
		q->header->aspect_ratio_type = 1;
	else
		q->header->aspect_ratio_type = 0;
	return true;
}

bool shared_queue_set_recommended_format(share_queue* q, int width, int height)
{
	if (!q || !q->header)
		return false;

	q->header->recommended_width = width;
	q->header->recommended_height = height;

	return true;
}