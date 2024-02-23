#include "ga_loadgen.h"

// Define a mock frame generator function
void generate_mock_frame(char *buf, int buflen, struct gaRect *rect) {
    // For demonstration purposes, let's fill the buffer with a constant color (e.g., white).
    memset(buf, 150, buflen);
}

int ga_loadgen_init(gaImage *gaimg) {
    // Set some arbitrary values for synthetic frame dimensions
    const int width = 640;
    const int height = 480;

    gaimg->height = height;
    gaimg->width = width;
    gaimg->bytes_per_line = width * 4;

    return 0;
}

int ga_loadgen_capture(char *buf, int buflen, struct gaRect *rect) {
    // Call the mock frame generator instead of capturing from X server
    generate_mock_frame(buf, buflen, rect);
    return 0;
}

void ga_loadgen_deinit() {}