#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/platform.h>
#include <util/threading.h>
#include <QMainWindow>
#include <QAction>
#include "../queue/share_queue.h"
#include "virtual_output.h"
#include "virtual_properties.h"
#include "get_format.h"
#include "math.h"


struct virtual_sink {
	obs_output_t *output;
	share_queue video_queue;
	share_queue audio_queue;
	int32_t width;
	int32_t height;
	int32_t delay;
	int64_t last_video_ts;
};

VirtualProperties* virtual_prop;
obs_output_t *virtual_out;
bool output_running=false;

static const char *virtual_output_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("VirtualOutput");
}

static void *virtual_output_create(obs_data_t *settings, obs_output_t *output)
{
	virtual_sink *data = 
		(virtual_sink *)bzalloc(sizeof(struct virtual_sink));
	
	data->output = output;
	data->delay = obs_data_get_int(settings, "delay_frame");
	UNUSED_PARAMETER(settings);
	return data;
}

static void virtual_output_destroy(void *data)
{
	virtual_sink *output = (virtual_sink*)data;
	if (output)
		bfree(data);
}

static bool virtual_output_start(void *data)
{
	virtual_sink* out = (virtual_sink*)data;
	video_t *video = obs_output_video(out->output);
	const struct video_output_info *voi = video_output_get_info(video);
	out->width = (int32_t)obs_output_get_width(out->output);
	out->height = (int32_t)obs_output_get_height(out->output);
	AVPixelFormat fmt = obs_to_ffmpeg_video_format(
		video_output_get_format(video));
	double fps = video_output_get_frame_rate(video);
	int64_t frame_interval = static_cast<int64_t>(1000000000 / fps);

	bool start = shared_queue_create(&out->video_queue, ModeVideo,
		fmt, out->width, out->height, frame_interval,out->delay+10);

	start |= shared_queue_create(&out->audio_queue, 
		ModeAudio, fmt, AUDIO_SIZE, 1, frame_interval,
		video_frame_to_audio_frame(fps, out->delay + 10, 44100 * 4, AUDIO_SIZE));

	struct audio_convert_info conv = {};
	conv.format = AUDIO_FORMAT_16BIT;
	conv.samples_per_sec = 44100;
	conv.speakers = SPEAKERS_STEREO;

	obs_output_set_audio_conversion(out->output, &conv);

	if (start){
		shared_queue_set_delay(&out->video_queue, out->delay);
		shared_queue_set_delay(&out->audio_queue, out->delay);
		start = obs_output_begin_data_capture(out->output, 0);
	}

	return start;
}

static void virtual_output_stop(void *data, uint64_t ts)
{
	virtual_sink *out = (virtual_sink*)data;
	obs_output_end_data_capture(out->output);
	shared_queue_close(&out->video_queue);
	shared_queue_close(&out->audio_queue);
}

static void virtual_video(void *param, struct video_data *frame)
{
	virtual_sink *output = (virtual_sink*)param;
	output->last_video_ts = frame->timestamp;
	shared_queue_push_video(&output->video_queue, frame->linesize,
		output->height, frame->data, frame->timestamp);

}

static void virtual_audio(void *param, struct audio_data *frame)
{
	virtual_sink *output = (virtual_sink*)param;
	uint64_t ts = frame->timestamp;
	shared_queue_push_audio(&output->audio_queue, frame->frames*4,
		frame->data[0], ts, output->last_video_ts);
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
		output_info.get_properties = virtual_getproperties;

		return output_info;
}

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-virtual_output", "en-US")

bool obs_module_load(void)
{
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

	obs_output_info virtual_output_info = create_output_info();
	obs_register_output(&virtual_output_info);

	return true;
}

void obs_module_unload(void)
{
}

void virtual_output_enable(int delay)
{
	if (!output_running){
		obs_data_t *settings = obs_data_create();
		obs_data_set_int(settings, "delay_frame", delay);
		virtual_out = obs_output_create("virtual_output", "VirtualOutput",
			settings, NULL);
		obs_output_addref(virtual_out);
		bool a = obs_output_start(virtual_out);
		output_running = true;
		obs_data_release(settings);
	}
}

void virtual_output_disable()
{
	if (output_running){
		obs_output_stop(virtual_out);
		obs_output_release(virtual_out);
		output_running = false;
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