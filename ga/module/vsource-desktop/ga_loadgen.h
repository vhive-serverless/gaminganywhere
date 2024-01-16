#ifndef ___XCAP_LOADGEN_H__
#define ___XCAP_LOADGEN_H__

#ifndef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ga-common.h"

void generate_mock_frame(char *buf, int buflen, struct gaRect *rect);
int ga_loadgen_init(struct gaImage *image);
void ga_loadgen_deinit();
int ga_loadgen_capture(char *buf, int buflen, struct gaRect *rect);

#ifndef __cplusplus
}
#endif

#endif