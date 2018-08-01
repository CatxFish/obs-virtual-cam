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
		q_head->canvas_width = width;
		q_head->canvas_height = height;
		q->mode = mode;
		q->index = 0;
		q->operating_width = width;
		q->operating_height = height;
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

void copy_video(uint8_t* dst, uint8_t* src, uint32_t linesize, uint32_t height,
	uint32_t width)
{
	for (int i = 0; i < height; i++) {
		memcpy(dst, src, width);
		dst += width;
		src += linesize;
	}
}

bool shared_queue_push_video(share_queue* q, uint32_t* linesize,
	uint32_t height, uint8_t** data, uint64_t timestamp, int* crop)
{
	if (!q || !q->header)
		return false;

	frame_header* frame = get_frame_header(q->header, q->index);
	uint8_t* dst = (uint8_t*)frame + q->header->element_header_size;
	uint8_t* src[4];
	uint32_t width = q->operating_width - crop[0] - crop[2];
	int planes = 0;
	int fmt = q->header->format;
	height = height - crop[1] - crop[3];
	

	switch (fmt) {
	case AV_PIX_FMT_NONE:
		return false;

	case AV_PIX_FMT_YUV420P:
		src[0] = data[0] + crop[1] * linesize[0] + crop[0];
		src[1] = data[1] + crop[1] * linesize[1] / 2 + crop[0] / 2;
		src[2] = data[2] + crop[1] * linesize[2] / 2 + crop[0] / 2;
		frame->linesize[0] = width;
		frame->linesize[1] = width / 2;
		frame->linesize[2] = width / 2;	
		copy_video(dst, src[0], linesize[0], height, width);
		dst += width * height;		
		copy_video(dst, src[1], linesize[1], height / 2, width / 2);
		dst += width * height / 4;
		copy_video(dst, src[2], linesize[2], height / 2, width / 2);
		break;

	case AV_PIX_FMT_NV12:
		src[0] = data[0] + crop[1] * linesize[0] + crop[0];
		src[1] = data[1] + crop[1] * linesize[1] / 2 + crop[0];
		frame->linesize[0] = width;
		frame->linesize[1] = width;
		copy_video(dst, src[0], linesize[0], height, width);
		dst += width * height;
		copy_video(dst, src[1], linesize[1], height / 2, width);
		break;

	case AV_PIX_FMT_GRAY8:
		src[0] = data[0] + crop[1] * linesize[0] + crop[0];
		frame->linesize[0] = width;
		copy_video(dst, src[0], linesize[0], height, width);
		break;

	case AV_PIX_FMT_YUYV422:
	case AV_PIX_FMT_UYVY422:
		src[0] = data[0] + crop[1] * linesize[0] + crop[0] * 2;
		frame->linesize[0] = width * 2;
		copy_video(dst, src[0], linesize[0], height, width * 2);
		break;

	case AV_PIX_FMT_RGBA:
	case AV_PIX_FMT_BGRA:
		src[0] = data[0] + crop[1] * linesize[0] + crop[0] * 4;
		frame->linesize[0] = width * 4;
		copy_video(dst, src[0], linesize[0], height, width * 4);
		break;

	case AV_PIX_FMT_YUV444P:
		src[0] = data[0] + crop[1] * linesize[0] + crop[0];
		src[1] = data[1] + crop[1] * linesize[1] + crop[0];
		src[2] = data[2] + crop[1] * linesize[2] + crop[0];
		frame->linesize[0] = width;
		frame->linesize[1] = width;
		frame->linesize[2] = width;
		copy_video(dst, src[0], linesize[0], height, width);
		dst += width * height;
		copy_video(dst, src[1], linesize[1], height, width);
		dst += width * height;
		copy_video(dst, src[2], linesize[2], height, width);
		break;
	}

	frame->timestamp = timestamp;
	frame->frame_width = q->operating_width - crop[0] - crop[2];
	frame->frame_height = q->operating_height - crop[1] - crop[3];

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

	uint8_t* buff = (uint8_t*)q->header + offset;
	frame_header* head = (frame_header*)buff;
	uint8_t* data = (uint8_t*)buff + q->header->element_header_size;
	memcpy(data, src, size);
	head->linesize[0] = size;
	head->timestamp = timestamp;

	q->header->last_ts = video_ts;
	q->header->write_index = q->index;
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