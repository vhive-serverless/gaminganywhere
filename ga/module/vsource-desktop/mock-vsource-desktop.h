#ifndef __ISOURCE_DESKTOP_H__
#define __ISOURCE_DESKTOP_H__

#include "ga-module.h"
#include "ga_loadgen.h"

#if 0
MODULE MODULE_EXPORT int vsource_init(void *arg);		// arg is pipeline format, e.g., image-%d
MODULE MODULE_EXPORT void * vsource_threadproc(void *arg);	// arg is pipeline format, e.g., image-%d
MODULE MODULE_EXPORT void vsource_deinit(void *arg);		// arg is not used
#endif

#endif