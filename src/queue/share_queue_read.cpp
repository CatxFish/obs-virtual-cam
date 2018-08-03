#include "share_queue_read.h"

bool shared_queue_open(share_queue* q, int mode)
{
	if (!q)
		return false;

	const char* name = get_mapping_name(mode);

	q->hwnd = OpenFileMappingA(FILE_MAP_READ, FALSE, name);

	if (q->hwnd) {
		q->header = (queue_header*)MapViewOfFile(q->hwnd, FILE_MAP_READ, 0, 0, 
			0);
	} else
		return false;

	if (q->header == nullptr) {
		CloseHandle(q->hwnd);
		q->hwnd = nullptr;
		return false;
	}

	q->mode = mode;
	return true;
}

void shared_queue_read_close(share_queue* q, dst_scale_context* scale_info)
{
	if (q && q->header) {
		UnmapViewOfFile(q->header);
		CloseHandle(q->hwnd);
		q->header = nullptr;
		q->hwnd = NULL;
		q->index = -1;
	}

	if (scale_info) {
		sws_freeContext(scale_info->convert_ctx);
		scale_info->convert_ctx = nullptr;
	}
}

bool share_queue_init_index(share_queue* q)
{
	if (!q || !q->header)
		return false;


	if (q->mode < ModeAudio) {
		q->index = q->header->write_index - q->header->delay_frame;
		if (q->index < 0)
			q->index += q->header->queue_length;

	} else if (q->mode == ModeAudio) {
		int write_index = q->header->write_index;
		int delay_frame = q->header->delay_frame;
		int64_t frame_time = q->header->frame_time;
		int64_t last_ts = q->header->last_ts;
		uint64_t start_ts = last_ts - delay_frame*frame_time;

		int index = write_index;
		uint64_t frame_ts = 0;

		do {
			index--;
			if (index < 0)
				index += q->header->queue_length;

			frame_header* frame = get_frame_header(q->header, index);
			frame_ts = frame->timestamp;

		} while (frame_ts > start_ts && index != write_index);

		if (index == write_index) {
			q->index = write_index - q->header->queue_length / 2;
			if (q->index < 0)
				q->index += q->header->queue_length;
		} else
			q->index = index;
	}

	return true;
}

bool shared_queue_get_video_format(int mode, int* format, uint32_t* width,
	uint32_t* height, uint64_t* avgtime)
{
	bool success = true;
	HANDLE hwnd;
	queue_header* header;

	if (mode < ModeVideo || mode >= ModeAudio)
		return false;

	const char* name = get_mapping_name(mode);

	hwnd = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, name);
	if (hwnd)
		header = (queue_header*)MapViewOfFile(hwnd, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	else
		return false;

	if (header) {
		*format = header->format;
		*avgtime = (header->frame_time) / 100;
		*width = header->recommended_width;
		*height = header->recommended_height;
		UnmapViewOfFile(header);
	} else
		success = false;

	if (*height <= 0 || *width <= 0 || *avgtime <= 0)
		success = false;

	CloseHandle(hwnd);
	return success;
}

void clear_image(uint8_t* dst, int width, int height, int linesize)
{
	int black_value = 0x80008000;
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width / 2; j++) {
			memcpy(dst + j * 4, &black_value, 4);
		}
		dst += linesize;
	}
}

void set_convert_context(share_queue* q, dst_scale_context* scale_info,
	int src_width,int src_height)
{
	int dst_width = 0;
	int dst_height = 0;

	if (q->header->aspect_ratio_type == 0) {
		dst_width = scale_info->dst_width;
		dst_height = scale_info->dst_height;
		scale_info->dst_offset = 0;
	} else {
		double x_ratio = (double)scale_info->dst_width / src_width;
		double y_ratio = (double)scale_info->dst_height / src_height;
		if (x_ratio > y_ratio) {
			dst_width = min(scale_info->dst_width, src_width * y_ratio);
			dst_width = (dst_width >> 3) << 3; //align 8
			dst_height = scale_info->dst_height;
			scale_info->dst_offset = ((scale_info->dst_width - dst_width)
				>> 2) << 2;
		} else if(x_ratio < y_ratio) {
			dst_height = min(scale_info->dst_height, src_height * x_ratio);
			dst_width = scale_info->dst_width;
			scale_info->dst_offset = (scale_info->dst_height - dst_height) / 2 
				* scale_info->dst_linesize[0];
		} else {
			dst_width = scale_info->dst_width;
			dst_height = scale_info->dst_height;
			scale_info->dst_offset = 0;
		}
	}

	scale_info->convert_ctx = sws_getContext(src_width, src_height,
		(AVPixelFormat)q->header->format, dst_width, dst_height,
		(AVPixelFormat)scale_info->dst_format, SWS_FAST_BILINEAR,
		NULL, NULL, NULL);
	q->operating_width = src_width;
	q->operating_height = src_height;
}

bool shared_queue_get_video(share_queue* q, dst_scale_context* scale_info,
	uint8_t* dst,uint64_t* timestamp)
{
	if (!q || !q->header)
		return false;

	if (q->index == q->header->write_index)
		return false;

	if (q->index < 0)
		share_queue_init_index(q);
	
	uint8_t* data[4];

	frame_header* frame = get_frame_header(q->header, q->index);
	uint8_t* buff = (uint8_t*)frame + q->header->element_header_size;
	int height = frame->frame_height;
	int planes = 0;

	switch (q->header->format) {
	case AV_PIX_FMT_NONE:
		return false;

	case AV_PIX_FMT_YUV420P:
		data[0] = buff;
		data[1] = data[0] + frame->linesize[0] * height;
		data[2] = data[1] + frame->linesize[1] * height / 2;
		break;

	case AV_PIX_FMT_NV12:
		data[0] = buff;
		data[1] = data[0] + frame->linesize[0] * height;
		break;

	case AV_PIX_FMT_GRAY8:
	case AV_PIX_FMT_YUYV422:
	case AV_PIX_FMT_UYVY422:
	case AV_PIX_FMT_RGBA:
	case AV_PIX_FMT_BGRA:
		data[0] = buff;
		break;

	case AV_PIX_FMT_YUV444P:
		data[0] = buff;
		data[1] = data[0] + frame->linesize[0] * height;
		data[2] = data[1] + frame->linesize[1] * height;
		break;
	}

	if (!scale_info->convert_ctx ||
	    frame->frame_width != q->operating_width ||
	    frame->frame_height != q->operating_height||
	    q->header->aspect_ratio_type != scale_info->aspect_ratio_type) {
		sws_freeContext(scale_info->convert_ctx);
		set_convert_context(q, scale_info, frame->frame_width,
			frame->frame_height);
		scale_info->aspect_ratio_type = q->header->aspect_ratio_type;
		clear_image(dst, scale_info->dst_width, scale_info->dst_height,
			scale_info->dst_linesize[0]);
	}

	dst += scale_info->dst_offset;

	sws_scale(scale_info->convert_ctx,
		(const uint8_t *const *)data,
		(const int*)frame->linesize, 0, q->operating_height,
		(uint8_t *const *)&dst, (const int*)scale_info->dst_linesize);

	*timestamp = frame->timestamp;

	q->index++;

	if (q->index >= q->header->queue_length)
		q->index = 0;

	return true;
}



bool shared_queue_get_audio(share_queue* q, uint8_t* dst,
	uint32_t max_size, uint64_t* timestamp)
{
	if (!q || !q->header)
		return false;

	if (q->index == q->header->write_index)
		return false;

	if (q->index < 0)
		share_queue_init_index(q);

	frame_header* frame = get_frame_header(q->header, q->index);
	uint8_t* src = (uint8_t*)frame + q->header->element_header_size;

	if (frame->linesize[0] <= max_size) {
		memcpy(dst, src, frame->linesize[0]);
		q->operating_width = frame->linesize[0];
	} else {
		memset(dst, 0, q->operating_width);
		q->operating_width = max_size;
	}

	*timestamp = frame->timestamp;

	q->index++;

	if (q->index >= q->header->queue_length)
		q->index = 0;

	return true;
}