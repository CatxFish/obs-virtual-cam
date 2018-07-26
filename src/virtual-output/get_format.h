#include "libavutil/pixfmt.h"
#include <media-io/video-io.h>

#define AV_SAMPLE_FMT_S16 1

static inline enum AVPixelFormat obs_to_ffmpeg_video_format(
enum video_format format)
{
	switch (format) {
		case VIDEO_FORMAT_NONE: return AV_PIX_FMT_NONE;
		case VIDEO_FORMAT_I444: return AV_PIX_FMT_YUV444P;
		case VIDEO_FORMAT_I420: return AV_PIX_FMT_YUV420P;
		case VIDEO_FORMAT_NV12: return AV_PIX_FMT_NV12;
		case VIDEO_FORMAT_YVYU: return AV_PIX_FMT_NONE;
		case VIDEO_FORMAT_YUY2: return AV_PIX_FMT_YUYV422;
		case VIDEO_FORMAT_UYVY: return AV_PIX_FMT_UYVY422;
		case VIDEO_FORMAT_RGBA: return AV_PIX_FMT_RGBA;
		case VIDEO_FORMAT_BGRA: return AV_PIX_FMT_BGRA;
		case VIDEO_FORMAT_BGRX: return AV_PIX_FMT_BGRA;
		case VIDEO_FORMAT_Y800: return AV_PIX_FMT_GRAY8;
	}
	return AV_PIX_FMT_NONE;
}
