#include <stdio.h>
#include <pthread.h>
#include "mock-vsource-desktop.h"
#include "vsource.h"
#include "rtspconf.h"
#include <unistd.h>
#include "encoder-common.h"


#define	SOURCES			1

using namespace std;

static struct gaRect croprect;
static struct gaRect *prect = NULL;
static int screenwidth, screenheight;

static struct gaImage realimage, *image = &realimage;

static int vsource_initialized = 0;
static int vsource_started = 0;
static pthread_t vsource_tid;

/* support reconfiguration of frame rate */
static int vsource_framerate_n = -1;
static int vsource_framerate_d = -1;
static int vsource_reconfigured = 0;

/* video source has to send images to video-# pipes */
/* the format is defined in VIDEO_SOURCE_PIPEFORMAT */

/*
 * vsource_init(void *arg)
 * arg is a pointer to a gaRect (if cropping is enabled)
 */
static int
vsource_init(void *arg) {
	struct RTSPConf *rtspconf = rtspconf_global();
	struct gaRect *rect = (struct gaRect*) arg;
	//
	if(vsource_initialized != 0)
		return 0;
#ifdef ENABLE_EMBED_COLORCODE
	vsource_embed_colorcode_init(1/*RGBmode*/);
#endif
	if(rect != NULL) {
		if(ga_fillrect(&croprect, rect->left, rect->top, rect->right, rect->bottom) == NULL) {
			ga_error("video source: invalid rect (%d,%d)-(%d,%d)\n",
				rect->left, rect->top,
				rect->right, rect->bottom);
			return -1;
		}
		prect = &croprect;
	} else {
		prect = NULL;
	}

    if (ga_loadgen_init(image) < 0) {
        ga_error("Loadgen capture init failed.\n");
        return -1;
    }

	screenwidth = image->width;
	screenheight = image->height;

#ifdef SOURCES
	do {
		int i;
		vsource_config_t config[SOURCES];
		bzero(config, sizeof(config));
		for(i = 0; i < SOURCES; i++) {
			//config[i].rtp_id = i;
			config[i].curr_width = prect ? prect->width : image->width;
			config[i].curr_height = prect ? prect->height : image->height;
			config[i].curr_stride = prect ? prect->linesize : image->bytes_per_line;
		}
		if(video_source_setup_ex(config, SOURCES) < 0) {
			return -1;
		}
	} while(0);
#endif
	//
	vsource_initialized = 1;
	return 0;
}


/*
 * vsource_threadproc accepts no arguments
 */
static void *
vsource_threadproc(void *arg) {
	int i;
	int token;
	int frame_interval;
	struct timeval tv;
	dpipe_buffer_t *data;
	vsource_frame_t *frame;
	dpipe_t *pipe[SOURCES];
	struct timeval initialTv, lastTv, captureTv;
	struct RTSPConf *rtspconf = rtspconf_global();
	// reset framerate setup
	vsource_framerate_n = rtspconf->video_fps;
	vsource_framerate_d = 1;
	vsource_reconfigured = 0;
	//
	frame_interval = 1000000/rtspconf->video_fps;	// in the unif of us
	frame_interval++;
#ifdef ENABLE_EMBED_COLORCODE
	vsource_embed_colorcode_reset();
#endif
	for(i = 0; i < SOURCES; i++) {
		char pipename[64];
		snprintf(pipename, sizeof(pipename), VIDEO_SOURCE_PIPEFORMAT, i);
		if((pipe[i] = dpipe_lookup(pipename)) == NULL) {
			ga_error("video source: cannot find pipeline '%s'\n", pipename);
			exit(-1);
		}
	}
	//
	ga_error("video source thread started: tid=%ld\n", ga_gettid());
	gettimeofday(&initialTv, NULL);
	lastTv = initialTv;
	token = frame_interval;
	while(vsource_started != 0) {
		// encoder has not launched?
		if(encoder_running() == 0) {
#ifdef WIN32
			Sleep(1);
#else
			usleep(1000);
#endif
			gettimeofday(&lastTv, NULL);
			token = frame_interval;
			continue;
		}
		// token bucket based capturing
		gettimeofday(&captureTv, NULL);
		token += tvdiff_us(&captureTv, &lastTv);
		if(token > (frame_interval<<1)) {
			token = (frame_interval<<1);
		}
		lastTv = captureTv;
		//
		if(token < frame_interval) {
#ifdef WIN32
			Sleep(1);
#else
			usleep(1000);
#endif
			continue;
		}
		token -= frame_interval;
		// copy image 
		data = dpipe_get(pipe[0]);
		frame = (vsource_frame_t*) data->pointer;
#ifdef __APPLE__
		frame->pixelformat = AV_PIX_FMT_RGBA;
#else
		frame->pixelformat = AV_PIX_FMT_BGRA;
#endif
		if(prect == NULL) {
		////////////////////////////////////////
		frame->realwidth = screenwidth;
		frame->realheight = screenheight;
		frame->realstride = screenwidth<<2;
		frame->realsize = screenheight * frame->realstride;
		////////////////////////////////////////
		} else {
		////////////////////////////////////////
		frame->realwidth = prect->width;
		frame->realheight = prect->height;
		frame->realstride = prect->width<<2;
		frame->realsize = prect->height * frame->realstride;
		////////////////////////////////////////
		}
		frame->linesize[0] = frame->realstride/*frame->stride*/;
#ifdef WIN32
	#ifdef D3D_CAPTURE
		ga_win32_D3D_capture((char*) frame->imgbuf, frame->imgbufsize, prect);
	#elif defined DFM_CAPTURE
		ga_win32_DFM_capture((char*) frame->imgbuf, frame->imgbufsize, prect);
	#else
		ga_win32_GDI_capture((char*) frame->imgbuf, frame->imgbufsize, prect);
	#endif
#elif defined __APPLE__
		ga_osx_capture((char*) frame->imgbuf, frame->imgbufsize, prect);
#elif defined ANDROID
		ga_androidvideo_capture((char*) frame->imgbuf, frame->imgbufsize);
#else // X11
		ga_loadgen_capture((char*) frame->imgbuf, frame->imgbufsize, prect);
#endif
		// draw cursor
#ifdef WIN32
		ga_win32_draw_system_cursor(frame);
#endif
		//gImgPts++;
		frame->imgpts = tvdiff_us(&captureTv, &initialTv)/frame_interval;
		frame->timestamp = captureTv;
		// embed color code?
#ifdef ENABLE_EMBED_COLORCODE
		vsource_embed_colorcode_inc(frame);
#endif
		// duplicate from channel 0 to other channels
		for(i = 1; i < SOURCES; i++) {
			dpipe_buffer_t *dupdata;
			vsource_frame_t *dupframe;
			dupdata = dpipe_get(pipe[i]);
			dupframe = (vsource_frame_t*) dupdata->pointer;
			//
			vsource_dup_frame(frame, dupframe);
			//
			dpipe_store(pipe[i], dupdata);
		}
		dpipe_store(pipe[0], data);
		// reconfigured?
		if(vsource_reconfigured != 0) {
			frame_interval = (int) (1000000.0 * vsource_framerate_d / vsource_framerate_n);
			frame_interval++;
			vsource_reconfigured = 0;
			ga_error("video source: reconfigured - framerate=%d/%d (interval=%d)\n",
				vsource_framerate_n, vsource_framerate_d, frame_interval);
		}
	}
	//
	ga_error("video source: thread terminated.\n");
	//
	return NULL;
}

static int
vsource_deinit(void *arg) {
	if(vsource_initialized == 0)
		return 0;
#ifdef WIN32
	#ifdef D3D_CAPTURE
	ga_win32_D3D_deinit();
	#elif defined DFM_CAPTURE
	ga_win32_DFM_deinit();
	#else
	ga_win32_GDI_deinit();
	#endif /* D3D_CAPTURE */
	CoUninitialize();
#elif defined __APPLE__
	ga_osx_deinit();
#elif defined ANDROID
	ga_androidvideo_deinit();
#else
	//ga_xwin_deinit(display, image);
	ga_loadgen_deinit();
#endif
	vsource_initialized = 0;
	return 0;
}

static int
vsource_start(void *arg) {
	if(vsource_started != 0)
		return 0;
	vsource_started = 1;
	if(pthread_create(&vsource_tid, NULL, vsource_threadproc, arg) != 0) {
		vsource_started = 0;
		ga_error("video source: create thread failed.\n");
		return -1;
	}
	pthread_detach(vsource_tid);
	return 0;
}

static int
vsource_stop(void *arg) {
	if(vsource_started == 0)
		return 0;
	vsource_started = 0;
	pthread_cancel(vsource_tid);
	return 0;
}

static int
vsource_ioctl(int command, int argsize, void *arg) {
	int ret = 0;
	ga_ioctl_reconfigure_t *reconf = (ga_ioctl_reconfigure_t*) arg;
	//
	if(vsource_initialized == 0)
		return GA_IOCTL_ERR_NOTINITIALIZED;
	//
	switch(command) {
	case GA_IOCTL_RECONFIGURE:
		if(argsize != sizeof(ga_ioctl_reconfigure_t))
			return GA_IOCTL_ERR_INVALID_ARGUMENT;
		if(reconf->framerate_n > 0 && reconf->framerate_d > 0) {
			double framerate;
			if(vsource_framerate_n == reconf->framerate_n
			&& vsource_framerate_d == reconf->framerate_d)
				break;
			framerate = 1.0 * reconf->framerate_n / reconf->framerate_d;
			if(framerate < 2 || framerate > 120) {
				return GA_IOCTL_ERR_INVALID_ARGUMENT;
			}
			vsource_framerate_n = reconf->framerate_n;
			vsource_framerate_d = reconf->framerate_d;
			vsource_reconfigured = 1;
		}
		break;
	default:
		ret = GA_IOCTL_ERR_NOTSUPPORTED;
		break;
	}
	return ret;
}

ga_module_t *
module_load() {
	static ga_module_t m;
	bzero(&m, sizeof(m));
	m.type = GA_MODULE_TYPE_VSOURCE;
	m.name = strdup("mock-vsource-desktop");
	m.init = vsource_init;
	m.start = vsource_start;
	m.stop = vsource_stop;
	m.deinit = vsource_deinit;
	m.ioctl = vsource_ioctl;
	return &m;
}