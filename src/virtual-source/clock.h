#include <windows.h>
#include <stdint.h>

uint64_t get_current_time(uint64_t start_time = 0);
bool sleepto(uint64_t time_target, uint64_t start_time);