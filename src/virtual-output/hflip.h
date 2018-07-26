extern "C"
{
#include "stdint.h"
#include "libavfilter/avfiltergraph.h"  
#include "libavfilter/buffersink.h"  
#include "libavfilter/buffersrc.h"  
#include "libavutil/avutil.h"  
#include "libavutil/imgutils.h" 
#include "libavformat/avformat.h"
};

struct FlipContext {
	bool init = false;
	AVFrame *frame_out = 0;
	AVFrame *frame_in = 0;
	unsigned char *frame_buffer_out = 0;
	AVFilterGraph *filter_graph = 0;
	AVFilterContext *buffersink_ctx = 0;
	AVFilterContext *buffersrc_ctx = 0;
};

bool init_flip_filter(FlipContext* ctx, int width, int height, int format);
bool release_flip_filter(FlipContext* ctx);
void flip_frame(FlipContext* ctx, uint8_t** src, uint32_t* linesize);
void unref_flip_frame(FlipContext* ctx);