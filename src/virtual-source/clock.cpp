#include "clock.h"

static bool have_clockfreq = false;
static LARGE_INTEGER clock_freq;

//directshow use 100ns as time unit

uint64_t get_current_time(uint64_t start_time)
{
	LARGE_INTEGER current_time;
	double time_val;

	if (!have_clockfreq) {
		QueryPerformanceFrequency(&clock_freq);
		have_clockfreq = true;
	}

	QueryPerformanceCounter(&current_time);
	time_val = (double)current_time.QuadPart;
	time_val *= 10000000.0;
	time_val /= (double)clock_freq.QuadPart;

	return (uint64_t)time_val - start_time;
}

bool sleepto(uint64_t time_target, uint64_t start_time)
{
	uint64_t t = get_current_time(start_time);
	uint32_t milliseconds;

	if (t >= time_target)
		return false;

	milliseconds = (uint32_t)((time_target - t) / 10000);
	if (milliseconds > 10)
		Sleep(milliseconds - 1);
}