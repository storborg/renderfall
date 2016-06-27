#pragma once

#include <stdint.h>

void start_progress(void);
void update_progress(uint32_t pos, uint32_t total);
void end_progress(void);
void shell_reset(void);
