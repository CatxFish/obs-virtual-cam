#include <obs-module.h>
#include <media-io/video-io.h>
#include <util/platform.h>
#include "../queue/share_queue_write.h"

#define S_DELAY             "delay"
#define S_START             "start"
#define S_STOP              "stop"
#define S_TARGET            "target"
#define S_FLIP              "flip"
#define S_RATIO             "keep-ratio"

#define T_(v)               obs_module_text(v)
#define T_DELAY             T_("DelayFrame")
#define T_START             T_("StartOutput")
#define T_STOP              T_("StopOutput")
#define T_TARGET            T_("Target")
#define T_FLIP              T_("HorizontalFlip")
#define T_RATIO             T_("KeepAspectRatio")

struct virtual_filter_data {
	obs_source_t* context = nullptr;
	bool active = false;
	bool flip = false;
	gs_texrender_t* texrender = nullptr;
	gs_stagesurf_t* stagesurface = nullptr;
	share_queue video_queue = {};
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t base_width = 0;
	uint32_t base_height = 0;
	int mode = 0;
	int delay = 0;
};

static const char *virtual_filter_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("VirtualCam");
}

static void *virtual_filter_create(obs_data_t *settings, obs_source_t *context)
{
	virtual_filter_data *data =
		(virtual_filter_data *)bzalloc(sizeof(struct virtual_filter_data));

	data->active = false;
	data->context = context;
	data->texrender = gs_texrender_create(GS_BGRA, GS_ZS_NONE);
	obs_source_update(context, settings);
	UNUSED_PARAMETER(settings);
	return data;
}

static void virtual_filter_video(void *param, uint32_t cx, uint32_t cy)
{
	virtual_filter_data* filter = (virtual_filter_data*)param;
	obs_source_t* target = obs_filter_get_target(filter->context);
	uint8_t* video_data;
	uint32_t linesize;
	uint32_t width = obs_source_get_width(target);
	uint32_t height = obs_source_get_height(target);
	uint64_t time = os_gettime_ns();
	struct vec4 background;

	if (!target)
		return;

	width = min(filter->base_width, width);
	height = min(filter->base_height, height);

	if (width == 0 || height == 0) {
		width = filter->base_width;
		height = filter->base_height;
	}

	gs_texrender_reset(filter->texrender);

	if (!gs_texrender_begin(filter->texrender, width, height))
		return;

	vec4_zero(&background);
	gs_clear(GS_CLEAR_COLOR, &background, 0.0f, 0);
	gs_ortho(0.0f, (float)width, 0.0f, (float)height, -100.0f, 100.0f);

	if(filter->flip) {
		gs_matrix_scale3f(-1.0f, 1.0f, 1.0f);
		gs_matrix_translate3f(-(float)width, 0.0f, 0.0f);
	}

	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
	obs_source_video_render(target);
	gs_blend_state_pop();
	gs_texrender_end(filter->texrender);

	if (filter->width != width || filter->height != height) {
		gs_stagesurface_destroy(filter->stagesurface);
		filter->stagesurface = gs_stagesurface_create(width, height, GS_BGRA);
		filter->width = width;
		filter->height = height;
		shared_queue_set_recommended_format(&filter->video_queue, width, 
			height);
	}

	gs_stage_texture(filter->stagesurface, gs_texrender_get_texture(
		filter->texrender));

	gs_stagesurface_map(filter->stagesurface, &video_data, &linesize);
	shared_queue_push_video(&filter->video_queue, &linesize, width, height,
		&video_data, time);
	gs_stagesurface_unmap(filter->stagesurface);

	UNUSED_PARAMETER(cx);
	UNUSED_PARAMETER(cy);
}


static void virtual_filter_destroy(void *data)
{
	virtual_filter_data *filter = (virtual_filter_data *)data;
	if (filter) {
		obs_remove_main_render_callback(virtual_filter_video, data);
		shared_queue_write_close(&filter->video_queue);
		obs_enter_graphics();
		gs_stagesurface_destroy(filter->stagesurface);
		gs_texrender_destroy(filter->texrender);
		obs_leave_graphics();
		bfree(filter);
	}
}

static void virtual_filter_update(void* data, obs_data_t* settings)
{
	virtual_filter_data *filter = (virtual_filter_data *)data;
	bool keep_ratio = obs_data_get_bool(settings, S_RATIO);
	filter->delay = (int)obs_data_get_int(settings, S_DELAY);
	filter->mode = (int)obs_data_get_int(settings, S_TARGET);
	filter->flip = obs_data_get_bool(settings, S_FLIP);
	shared_queue_set_keep_ratio(&filter->video_queue, keep_ratio);	
}

static void virtual_filter_render(void* data, gs_effect_t* effect)
{
	virtual_filter_data *filter = (virtual_filter_data *)data;
	obs_source_skip_video_filter(filter->context);
	UNUSED_PARAMETER(effect);
}

static bool virtual_filter_start(obs_properties_t *props, obs_property_t *p,
	void *data)
{
	virtual_filter_data* filter = (virtual_filter_data*)data;
	obs_source_t* target = obs_filter_get_target(filter->context);
	struct obs_video_info ovi = { 0 };
	uint32_t base_width, base_height;
	uint64_t interval;
	
	obs_get_video_info(&ovi);
	base_width = obs_source_get_base_width(target);
	base_height = obs_source_get_base_height(target);

	if (base_width > 0 && base_height > 0) {
		filter->base_width = max(ovi.base_width, base_width);
		filter->base_height = max(ovi.base_height, base_height);
		filter->width = 0;
		filter->height = 0;
		interval = (uint64_t)ovi.fps_den * 1000000000ULL / (uint64_t)ovi.fps_num;
		filter->active = shared_queue_create(&filter->video_queue, filter->mode,
			AV_PIX_FMT_BGRA, filter->base_width, filter->base_height, interval,
			filter->delay + 10);
	} else {
		blog(LOG_WARNING, "virtual-filter target size error");
		filter->active = false;
	}

	if (filter->active) {
		obs_property_t *stop = obs_properties_get(props, S_STOP);
		obs_property_set_visible(p, false);
		obs_property_set_visible(stop, true);
		shared_queue_set_delay(&filter->video_queue, filter->delay);
		obs_add_main_render_callback(virtual_filter_video, data);
		blog(LOG_INFO, "starting virtual-filter on VirtualCam'%d'",
			filter->mode + 1);
	} else {
		blog(LOG_WARNING, "starting virtual-filter failed on VirtualCam'%d'",
			filter->mode + 1);
	}

	return filter->active;
}

static bool virtual_filter_stop(obs_properties_t *props, obs_property_t *p,
	void *data)
{
	virtual_filter_data* filter = (virtual_filter_data*)data;
	obs_remove_main_render_callback(virtual_filter_video, data);
	shared_queue_write_close(&filter->video_queue);
	obs_property_t *start = obs_properties_get(props, S_START);
	filter->active = false;
	obs_property_set_visible(p, false);
	obs_property_set_visible(start, true);
	blog(LOG_INFO, "virtual-filter stop");
	return true;
}

static obs_properties_t *virtual_filter_properties(void *data)
{
	virtual_filter_data *filter = (virtual_filter_data*)data;
	obs_properties_t *props = obs_properties_create();
	obs_property *start, *stop, *cb;
	obs_properties_add_int_slider(props, S_DELAY, T_DELAY, 0, 30, 1);
	cb = obs_properties_add_list(props, S_TARGET, T_TARGET, 
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(cb, "OBS-Camera", ModeVideo);
	obs_property_list_add_int(cb, "OBS-Camera2", ModeVideo2);
	obs_property_list_add_int(cb, "OBS-Camera3", ModeVideo3);
	obs_property_list_add_int(cb, "OBS-Camera4", ModeVideo4);

	obs_properties_add_bool(props, S_FLIP, T_FLIP);
	obs_properties_add_bool(props, S_RATIO, T_RATIO);

	start = obs_properties_add_button(props, S_START, T_START, 
		virtual_filter_start);
	stop = obs_properties_add_button(props, S_STOP, T_STOP, 
		virtual_filter_stop);

	obs_property_set_visible(start, !filter->active);
	obs_property_set_visible(stop, filter->active);

	return props;
}

static void virtual_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, S_DELAY, 1);
}

struct obs_source_info create_filter_info()
{
	struct obs_source_info filter_info = {};
	filter_info.id = "virtualcam-filter";
	filter_info.type = OBS_SOURCE_TYPE_FILTER;
	filter_info.output_flags = OBS_SOURCE_VIDEO;
	filter_info.get_name = virtual_filter_get_name;
	filter_info.create = virtual_filter_create;
	filter_info.destroy = virtual_filter_destroy;
	filter_info.update = virtual_filter_update;
	filter_info.video_render = virtual_filter_render;
	filter_info.get_properties = virtual_filter_properties;
	filter_info.get_defaults = virtual_filter_defaults;
	return filter_info;
}

void virtual_filter_init()
{
	obs_source_info virtual_filter_info = create_filter_info();
	obs_register_source(&virtual_filter_info);
}