#include <stdint.h>
#include <stdio.h>

void start_progress(void) {
    // Hide cursor
    printf("\033[?25l");
}

void update_progress(uint32_t pos, uint32_t total) {
    if (pos % (total / 100) == 0) {
        printf("\rProgress: %d%%", ((100 * pos) / total));
        fflush(stdout);
    }
}

void end_progress(void) {
    // Overwrite the line we were showing progress on.
    printf("\r                          \r");
    // Show cursor again
    printf("\033[?25h");
    fflush(stdout);
}

// XXX We want to call this if we get interrupted somehow, like on a ctrl-C.
void shell_reset(void) {
    printf("\033[?25h");
    fflush(stdout);
}
