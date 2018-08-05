#ifndef VIRTUAL_OUTPUT_H
#define VIRTUAL_OUTPUT_H

#include <util/threading.h>

void virtual_output_init();
void virtual_output_terminate();
void virtual_output_enable();
void virtual_output_disable();
void virtual_output_set_data(struct vcam_update_data* update_data);
int video_frame_to_audio_frame(double video_fps, int video_frame, 
	int audio_sample_rate, int audio_frame_size);
bool virtual_output_is_running();
void virtual_signal_connect(const char *signal, signal_callback_t callback,
	void *data);
void virtual_signal_disconnect(const char *signal, signal_callback_t callback,
	void *data);

#endif