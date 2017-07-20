#ifndef VIRTUAL_OUTPUT_H
#define VIRTUAL_OUTPUT_H


void virtual_output_enable(int delay);
void virtual_output_disable();
int video_frame_to_audio_frame(double video_fps, int video_frame, 
	int audio_sample_rate, int audio_frame_size);
bool virtual_output_is_running();
bool virtual_output_occupied();

#endif