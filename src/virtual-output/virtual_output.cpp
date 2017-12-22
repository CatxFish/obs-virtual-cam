#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/platform.h>
#include <util/threading.h>
#include <util/config-file.h>
#include <QMainWindow>
#include <QAction>
#include "../queue/share_queue_write.h"
#include "virtual_output.h"
#include "virtual_properties.h"
#include "get_format.h"
#include "hflip.h"
#include "math.h"

#define round_even(x){x&(-2)}

struct virtual_out_data {
	obs_output_t *output = nullptr;
	pthread_mutex_t mutex;
	share_queue video_queue;
	share_queue audio_queue;
	int width = 0;
	int height = 0;
	int delay = 0;
	int crop[4]; //left,top,right,bottom
	int64_t last_video_ts = 0;
	bool hori_flip;
	bool keep_ratio;
	FlipContext flip_ctx;
};

VirtualProperties* virtual_prop;
obs_output_t *virtual_out;
bool output_running=false;

static const char *virtual_output_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("VirtualOutput");
}

static void virtual_output_destroy(void *data)
{
	output_running = false;
	virtual_out_data *out_data = (virtual_out_data*)data;
	if (out_data){
		pthread_mutex_lock(&out_data->mutex);
		release_flip_filter(&out_data->flip_ctx);
		pthread_mutex_unlock(&out_data->mutex);
		pthread_mutex_destroy(&out_data->mutex);
		bfree(data);
	}
	
}
static void *virtual_output_create(obs_data_t *settings, obs_output_t *output)
{
	virtual_out_data *data =
		(virtual_out_data *)bzalloc(sizeof(struct virtual_out_data));
	
	data->output = output;
	data->delay = obs_data_get_int(settings, "delay_frame");
	pthread_mutex_init_value(&data->mutex);
	if (pthread_mutex_init(&data->mutex, NULL) == 0){
		UNUSED_PARAMETER(settings);
		return data;
	}
	else
		virtual_output_destroy(data);
	return NULL;
}

static bool virtual_output_start(void *data)
{
	virtual_out_data* out_data = (virtual_out_data*)data;
	video_t *video = obs_output_video(out_data->output);
	const struct video_output_info *voi = video_output_get_info(video);
	out_data->width = (int32_t)obs_output_get_width(out_data->output);
	out_data->height = (int32_t)obs_output_get_height(out_data->output);
	AVPixelFormat fmt = obs_to_ffmpeg_video_format(
		video_output_get_format(video));
	double fps = video_output_get_frame_rate(video);
	int64_t interval = static_cast<int64_t>(1000000000 / fps);

	bool start = shared_queue_create(&out_data->video_queue, ModeVideo,fmt,
		out_data->width, out_data->height, interval, out_data->delay + 10);

	start |= shared_queue_create(&out_data->audio_queue,
		ModeAudio, fmt, AUDIO_SIZE, 1, interval,
		video_frame_to_audio_frame(fps, out_data->delay + 10, 44100 * 4, AUDIO_SIZE));

	struct audio_convert_info conv = {};
	conv.format = AUDIO_FORMAT_16BIT;
	conv.samples_per_sec = 44100;
	conv.speakers = SPEAKERS_STEREO;

	obs_output_set_audio_conversion(out_data->output, &conv);

	init_flip_filter(&out_data->flip_ctx, out_data->width, out_data->height, fmt);

	if (start){
		shared_queue_set_delay(&out_data->video_queue, out_data->delay);
		shared_queue_set_delay(&out_data->audio_queue, out_data->delay);
		start = obs_output_begin_data_capture(out_data->output, 0);
	}

	return start;
}

static void virtual_output_stop(void *data, uint64_t ts)
{
	virtual_out_data *out_data = (virtual_out_data*)data;
	obs_output_end_data_capture(out_data->output);
	shared_queue_write_close(&out_data->video_queue);
	shared_queue_write_close(&out_data->audio_queue);
}

static void virtual_video(void *param, struct video_data *frame)
{
	if (!output_running)
		return;

	virtual_out_data *out_data = (virtual_out_data*)param;
	out_data->last_video_ts = frame->timestamp;
	pthread_mutex_lock(&out_data->mutex);
	if (out_data->hori_flip){
		flip_frame(&out_data->flip_ctx, frame->data, frame->linesize);
		shared_queue_push_video(&out_data->video_queue, 
			(uint32_t*)out_data->flip_ctx.frame_out->linesize,out_data->height, 
			out_data->flip_ctx.frame_out->data, frame->timestamp, out_data->crop);
		unref_flip_frame(&out_data->flip_ctx);
	}
	else{
		shared_queue_push_video(&out_data->video_queue, frame->linesize,
			out_data->height, frame->data, frame->timestamp, out_data->crop);
	}
	pthread_mutex_unlock(&out_data->mutex);

}

static void virtual_audio(void *param, struct audio_data *frame)
{
	if (!output_running)
		return;

	virtual_out_data *out_data = (virtual_out_data*)param;
	uint64_t ts = frame->timestamp;
	shared_queue_push_audio(&out_data->audio_queue, frame->frames * 4,
		frame->data[0], ts, out_data->last_video_ts);
}

static void virtual_output_update(void *data, obs_data_t *settings)
{
	virtual_out_data *out_data = (virtual_out_data*)data;
	obs_video_info vif;
	obs_get_video_info(&vif);
	int crop[4];
	int b_width = vif.base_width;
	int b_height = vif.base_height;
	bool hori_flip = obs_data_get_bool(settings, "hori-flip");
	bool keep_ratio = obs_data_get_bool(settings, "keep-ratio");

	if (hori_flip){
		crop[0] = obs_data_get_int(settings, "crop_right");
		crop[2] = obs_data_get_int(settings, "crop_left");
	}
	else{
		crop[0] = obs_data_get_int(settings, "crop_left");
		crop[2] = obs_data_get_int(settings, "crop_right");
	}
	
	crop[1] = obs_data_get_int(settings, "crop_top");
	crop[3] = obs_data_get_int(settings, "crop_bottom");
	crop[0] = round_even(crop[0] * out_data->width / b_width);
	crop[1] = round_even(crop[1] * out_data->height / b_height);
	crop[2] = round_even(crop[2] * out_data->width / b_width);
	crop[3] = round_even(crop[3] * out_data->height / b_height);

	if (crop[0] != out_data->crop[0] ||
		crop[1] != out_data->crop[1] ||
		crop[2] != out_data->crop[2] ||
		crop[3] != out_data->crop[3] ||
		hori_flip != out_data->hori_flip ||
		keep_ratio != out_data->keep_ratio)
	{
		pthread_mutex_lock(&out_data->mutex);
		out_data->hori_flip = hori_flip;
		out_data->keep_ratio = keep_ratio;
		for (int i = 0; i < 4; i++)
			out_data->crop[i] = crop[i];
		pthread_mutex_unlock(&out_data->mutex);
	}
}

obs_properties_t* virtual_getproperties(void *data)
{
	UNUSED_PARAMETER(data);

	obs_properties_t* props = obs_properties_create();
	obs_properties_set_flags(props, OBS_PROPERTIES_DEFER_UPDATE);

	obs_properties_add_text(props, "virtual_name",
		obs_module_text("Virtual.Name"), OBS_TEXT_DEFAULT);

	return props;
}

struct obs_output_info create_output_info()
{
		struct obs_output_info output_info = {};
		output_info.id = "virtual_output";
		output_info.flags = OBS_OUTPUT_AUDIO | OBS_OUTPUT_VIDEO;
		output_info.get_name = virtual_output_getname;
		output_info.create = virtual_output_create;
		output_info.destroy = virtual_output_destroy;
		output_info.start = virtual_output_start;
		output_info.stop = virtual_output_stop;
		output_info.raw_video = virtual_video;
		output_info.raw_audio = virtual_audio;
		output_info.update = virtual_output_update;
		output_info.get_properties = virtual_getproperties;

		return output_info;
}

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-virtual_output", "en-US")

bool obs_module_load(void)
{
	obs_output_info virtual_output_info = create_output_info();
	obs_register_output(&virtual_output_info);

	QMainWindow* main_window = (QMainWindow*)obs_frontend_get_main_window();
	QAction *action = (QAction*)obs_frontend_add_tools_menu_qaction(
		obs_module_text("VirtualCam"));

	obs_frontend_push_ui_translation(obs_module_get_string);
	virtual_prop = new VirtualProperties(main_window);
	obs_frontend_pop_ui_translation();

	auto menu_cb = []
	{
		virtual_prop->setVisible(!virtual_prop->isVisible());
	};

	action->connect(action, &QAction::triggered, menu_cb);

	return true;
}

void obs_module_unload(void)
{
}

void virtual_output_enable(int delay)
{
	if (delay < 0 || delay>30)
		delay = 3;

	if (!output_running){
		obs_data_t *settings = obs_data_create();
		obs_data_set_int(settings, "delay_frame", delay);
		virtual_out = obs_output_create("virtual_output", "VirtualOutput",
			settings, NULL);
		obs_output_addref(virtual_out);
		obs_output_start(virtual_out);
		output_running = true;
		obs_data_release(settings);
	}
}

void virtual_output_disable()
{
	if (output_running){
		output_running = false;
		obs_output_stop(virtual_out);
		obs_output_release(virtual_out);	
	}
}

void virtual_output_set_data(struct vcam_update_data* update_data)
{
	if (output_running){
		obs_data_t *settings = obs_data_create();
		obs_data_set_bool(settings, "hori-flip", update_data->horizontal_flip);
		obs_data_set_bool(settings, "keep-ratio", update_data->keep_ratio);
		obs_data_set_int(settings, "crop_left", update_data->crop[0]);
		obs_data_set_int(settings, "crop_top", update_data->crop[1]);
		obs_data_set_int(settings, "crop_right", update_data->crop[2]);
		obs_data_set_int(settings, "crop_bottom", update_data->crop[3]);
		obs_output_update(virtual_out, settings);
		obs_data_release(settings);
	}
}

bool virtual_output_is_running()
{
	return output_running;
}

bool virtual_output_occupied()
{
	if (virtual_output_is_running())
		return false;
	else
		return !(shared_queue_check(ModeVideo) && shared_queue_check(ModeAudio));
}

int video_frame_to_audio_frame(double video_fps, int video_frame, 
	int audio_sample_rate,int audio_frame_size)
{
	double duration = video_frame / video_fps;
	double size = audio_sample_rate * duration / audio_frame_size;
	return static_cast<int>(ceil(size));
}