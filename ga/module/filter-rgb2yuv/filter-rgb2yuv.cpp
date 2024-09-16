/*
 * Copyright (c) 2013-2014 Chun-Ying Huang
 *
 * This file is part of GamingAnywhere (GA).
 *
 * GA is free software; you can redistribute it and/or modify it
 * under the terms of the 3-clause BSD License as published by the
 * Free Software Foundation: http://directory.fsf.org/wiki/License:BSD_3Clause
 *
 * GA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the 3-clause BSD License along with GA;
 * if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <pthread.h>
#include <map>

#include "vsource.h"
#include "vconverter.h"
#include "rtspconf.h"
#include "encoder-common.h"

#include "ga-common.h"
#include "ga-conf.h"
#include "ga-avcodec.h"

#include "filter-rgb2yuv.h"

#define	POOLSIZE		8
#define	ENABLE_EMBED_COLORCODE	1

using namespace std;

static int filter_initialized = 0;
static int filter_started = 0;
static pthread_t filter_tid[VIDEO_SOURCE_CHANNEL_MAX];
static FILE *savefp = NULL;

/* filter_RGB2YUV_init: arg is two pointers to pipeline format string */
/*	1st ptr: source pipeline */
/*	2nd ptr: destination pipeline */
/* the number of pipeline(s) is equivalut to the number of video source(s) */

static int
filter_RGB2YUV_init(void *arg) {
	// arg is image source id
	int iid;
	const char **filterpipe = (const char **) arg;
	serverless_dpipe_t *srcpipe[VIDEO_SOURCE_CHANNEL_MAX];
	serverless_dpipe_t *dstpipe[VIDEO_SOURCE_CHANNEL_MAX];
	char savefile[128];
	//
	if(filter_initialized != 0)
		return 0;
	//

	if(ga_conf_readv("save-yuv-image", savefile, sizeof(savefile)) != NULL) {
		savefp = ga_save_init(savefile);
	}
	int maxHeight = VIDEO_SOURCE_DEF_MAXHEIGHT;
	int maxStride = VIDEO_SOURCE_DEF_MAXWIDTH*4;
	serverless_dpipe_create(0, "video-0", 8, sizeof(vsource_frame_t) * maxHeight * maxStride + 16);
	serverless_dpipe_create(0, "filter-0", 8, sizeof(vsource_frame_t) * maxHeight * maxStride + 16);
	
#ifdef ENABLE_EMBED_COLORCODE
	vsource_embed_colorcode_init(0/*RGBmode*/);
#endif
	//
	bzero(dstpipe, sizeof(dstpipe));
	//
	for(iid = 0; iid < video_source_channels(); iid++) {
		char pixelfmt[64];
		char srcpipename[64], dstpipename[64];
		int inputW, inputH, outputW, outputH;
		struct SwsContext *swsctx = NULL;
		serverless_dpipe_buffer_t *data = NULL;
		//
		snprintf(srcpipename, sizeof(srcpipename), filterpipe[0], iid);
		snprintf(dstpipename, sizeof(dstpipename), filterpipe[1], iid);
		srcpipe[iid] = serverless_dpipe_lookup(srcpipename);
		if(srcpipe[iid] == NULL) {
			char pipename[64];
			snprintf(pipename, sizeof(pipename), VIDEO_SOURCE_PIPEFORMAT, iid);
			int maxHeight = VIDEO_SOURCE_DEF_MAXHEIGHT;
			int maxStride = VIDEO_SOURCE_DEF_MAXWIDTH*4;
			serverless_dpipe_create(iid, pipename, 8, sizeof(vsource_frame_t) * maxHeight * maxStride + 16);
		}
		inputW = video_source_curr_width(iid);
		inputH = video_source_curr_height(iid);
		outputW = video_source_out_width(iid);
		outputH = video_source_out_height(iid);
		// create default converters
		ga_error("awefawefawef %s\n", pixelfmt);
		if(swsctx == NULL) {
#ifdef __APPLE__
			swsctx = create_frame_converter(
					inputW, inputH, AV_PIX_FMT_RGBA,
					outputW, outputH, AV_PIX_FMT_YUV420P);
#else
			swsctx = create_frame_converter(
					inputW, inputH, AV_PIX_FMT_BGRA,
					outputW, outputH, AV_PIX_FMT_YUV420P);
#endif
		}
		if(swsctx == NULL) {
			ga_error("RGB2YUV filter: cannot initialize converters.\n");
			goto init_failed;
		}
		//
		dstpipe[iid] = serverless_dpipe_create(iid, dstpipename, POOLSIZE,
				sizeof(vsource_frame_t) + video_source_mem_size(iid));
		if(dstpipe[iid] == NULL) {
			ga_error("RGB2YUV filter: create dst-pipeline failed (%s).\n", dstpipename);
			goto init_failed;
		}
		// for(data = dstpipe[iid]->in; data != NULL; data = data->next) {
		// 	if(vsource_frame_init(iid, (vsource_frame_t*) data->pointer) == NULL) {
		// 		ga_error("RGB2YUV filter: init frame failed for %s.\n", dstpipename);
		// 		goto init_failed;
		// 	}
		// }
		video_source_add_pipename(iid, dstpipename);
	}
	//
	filter_initialized = 1;
	//
	return 0;
init_failed:
	for(iid = 0; iid < video_source_channels(); iid++) {
		if(dstpipe[iid] != NULL)
			serverless_dpipe_destroy(dstpipe[iid]);
		dstpipe[iid] = NULL;
	}
#if 0
	if(pipe) {
		delete pipe;
	}
#endif
	return -1;
}

static int
filter_RGB2YUV_deinit(void *arg) {
	if(savefp != NULL) {
		ga_save_close(savefp);
		savefp = NULL;
	}
	return 0;
}

/* filter_RGB2YUV_threadproc: arg is two pointers to pipeline name */
/*	1st ptr: source pipeline */
/*	2nd ptr: destination pipeline */

static void *
filter_RGB2YUV_threadproc(void *arg) {
	// arg is pointer to source pipe
	//char pipename[64];
	const char **filterpipe = (const char **) arg;
	int maxHeight = VIDEO_SOURCE_DEF_MAXHEIGHT;
	int maxStride = VIDEO_SOURCE_DEF_MAXWIDTH*4;
	serverless_dpipe_t *srcpipe = serverless_dpipe_create(0, "video-0", 8, sizeof(vsource_frame_t) * maxHeight * maxStride + 16);
	serverless_dpipe_t *dstpipe = serverless_dpipe_create(0, "filter-0", 8, sizeof(vsource_frame_t) * maxHeight * maxStride + 16);
	// serverless_dpipe_t *srcpipe = serverless_dpipe_lookup(filterpipe[0]);
	// serverless_dpipe_t *dstpipe = serverless_dpipe_lookup(filterpipe[1]);
	serverless_dpipe_buffer_t *srcdata = NULL;
	serverless_dpipe_buffer_t *dstdata = NULL;
	vsource_frame_t *srcframe = NULL;
	vsource_frame_t *dstframe = NULL;
	// image info
	//int istride = video_source_maxstride();
	//
	unsigned char *src[] = { NULL, NULL, NULL, NULL };
	unsigned char *dst[] = { NULL, NULL, NULL, NULL };
	int srcstride[] = { 0, 0, 0, 0 };
	int dststride[] = { 0, 0, 0, 0 };
	int iid;
	int outputW, outputH;
	//
	struct SwsContext *swsctx = NULL;
	//
	pthread_mutex_t condMutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
	//
	if(srcpipe == NULL || dstpipe == NULL) {
		ga_error("RGB2YUV filter: bad pipeline (src=%p; dst=%p).\n", srcpipe, dstpipe);
		goto filter_quit;
	}
#ifdef ENABLE_EMBED_COLORCODE
	vsource_embed_colorcode_reset();
#endif
	//
	iid = dstpipe->channel_id;
	outputW = video_source_out_width(iid);
	outputH = video_source_out_height(iid);
	//
	ga_error("RGB2YUV filter[%ld]: pipe#%d from '%s' to '%s' (output-resolution=%dx%d)\n",
		ga_gettid(), iid,
		srcpipe->name, dstpipe->name,
		outputW/*iwidth*/, outputH/*iheight*/);
	// start filtering
	while(filter_started != 0) {
		// wait for notification
		srcdata = serverless_dpipe_load(srcpipe, NULL);
		if(srcdata == NULL) continue; // Polling here because no choice, redis is a bad idea
		srcframe = (vsource_frame_t*) srcdata->pointer;
		//
		dstdata = serverless_dpipe_get(dstpipe);
		dstdata->pointer = (vsource_frame_t*) malloc(sizeof(vsource_frame_t));
		dstframe = (vsource_frame_t*) dstdata->pointer;
		// basic info
		dstframe->imgpts = srcframe->imgpts;
		dstframe->timestamp = srcframe->timestamp;
		dstframe->pixelformat = AV_PIX_FMT_YUV420P;	//yuv420p;
		dstframe->realwidth = 640;
		dstframe->realheight = 480;
		dstframe->realstride = 640;
		dstframe->realsize = 640 * 480 * 3 / 2;
		// scale image: RGBA, BGRA, or YUV

		swsctx = lookup_frame_converter(
				srcframe->realwidth,
				srcframe->realheight,
				srcframe->pixelformat,
				dstframe->realwidth,
				dstframe->realheight,
				dstframe->pixelformat);
		if(swsctx == NULL) {
			swsctx = create_frame_converter(
				srcframe->realwidth,
				srcframe->realheight,
				srcframe->pixelformat,
				dstframe->realwidth,
				dstframe->realheight,
				dstframe->pixelformat);
		}
		if(swsctx == NULL) {
			ga_error("RGB2YUV filter: fatal - cannot create frame converter (%d,%d,%d)->(%x,%d,%d)\n",
				srcframe->realwidth, srcframe->realheight, srcframe->pixelformat,
				dstframe->realwidth, dstframe->realheight, dstframe->pixelformat);
		}
		srcframe->pixelformat = AV_PIX_FMT_BGRA;
		//
		if(srcframe->pixelformat == AV_PIX_FMT_RGBA
		|| srcframe->pixelformat == AV_PIX_FMT_BGRA/*rgba*/) {
			src[0] = srcframe->imgbuf;
			src[1] = NULL;
			srcstride[0] = srcframe->realstride; //srcframe->stride;
			srcstride[1] = 0;
		} else if(srcframe->pixelformat == AV_PIX_FMT_YUV420P) {
			src[0] = srcframe->imgbuf;
			src[1] = src[0] + ((srcframe->realwidth * srcframe->realheight));
			src[2] = src[1] + ((srcframe->realwidth * srcframe->realheight)>>2);
			src[3] = NULL;
			srcstride[0] = srcframe->linesize[0];
			srcstride[1] = srcframe->linesize[1];
			srcstride[2] = srcframe->linesize[2];
			srcstride[3] = NULL;
		} else {
			ga_error("filter-RGB2YUV: unsupported pixel format (%d)\n", srcframe->pixelformat);
			exit(-1);
		}
		AVFrame* bgraFrame = av_frame_alloc();
		AVFrame *yuvFrame = av_frame_alloc();

		bgraFrame->width = 640;
		bgraFrame->height = 480;
		bgraFrame->format = AV_PIX_FMT_BGRA;
		bgraFrame->data[0] = srcframe->imgbuf;
		bgraFrame->linesize[0] = 640 * 4;

		yuvFrame->width = 640;
		yuvFrame->height = 480;
		yuvFrame->format = AV_PIX_FMT_YUV420P;
		av_frame_get_buffer(yuvFrame, 32);
		sws_scale(swsctx, bgraFrame->data, bgraFrame->linesize, 0, 480, yuvFrame->data, yuvFrame->linesize);

		dstframe->imgbufsize = 640 * 480 + (640 / 2) * (480 / 2) * 2;
		dstframe->imgbuf = (unsigned char*) malloc (640 * 480 + (640 / 2) * (480 / 2) * 2);
		if (yuvFrame->data[0] == NULL) {
			continue;
		}
		memcpy(dstframe->imgbuf, yuvFrame->data[0], 640 * 480 + (640 / 2) * (480 / 2) * 2);
	
		// embed first, and then save
#ifdef ENABLE_EMBED_COLORCODE
		vsource_embed_colorcode_inc(dstframe);
#endif
		// only save the first channel
		if(iid == 0 && savefp != NULL) {
			ga_save_yuv420p(savefp, outputW, outputH, dst, dstframe->linesize);
		}
		//
		serverless_dpipe_put(srcpipe, srcdata);
		serverless_dpipe_store(dstpipe, dstdata);
		//
	}
	//
filter_quit:
	if(srcpipe) {
		srcpipe = NULL;
	}
	if(dstpipe) {
		serverless_dpipe_destroy(dstpipe);
		dstpipe = NULL;
	}
	//
	if(swsctx)	sws_freeContext(swsctx);
	//
	ga_error("RGB2YUV filter: thread terminated.\n");
	//
	return NULL;
}

/* filter_RGB2YUV_start: arg is two pointers to pipeline format string */
/*	1st ptr: source pipeline */
/*	2nd ptr: destination pipeline */
/* the number of pipeline(s) is equivalut to the number of video source(s) */

static int
filter_RGB2YUV_start(void *arg) {
	int iid;
	const char **filterpipe = (const char **) arg;
	static char *filter_param[VIDEO_SOURCE_CHANNEL_MAX][2];
#define	MAXPARAMLEN	64
	static char params[VIDEO_SOURCE_CHANNEL_MAX][2][MAXPARAMLEN];
	//
	if(filter_started != 0)
		return 0;
	filter_started = 1;
	for(iid = 0; iid < 1; iid++) {
		snprintf(params[iid][0], MAXPARAMLEN, filterpipe[0], iid);
		snprintf(params[iid][1], MAXPARAMLEN, filterpipe[1], iid);
		filter_param[iid][0] = params[iid][0];
		filter_param[iid][1] = params[iid][1];
		pthread_cancel_init();
		if(pthread_create(&filter_tid[iid], NULL, filter_RGB2YUV_threadproc, filter_param[iid]) != 0) {
			filter_started = 0;
			ga_error("filter RGB2YUV: create thread failed.\n");
			return -1;
		}
		pthread_detach(filter_tid[iid]);
	}
	return 0;
}

/* filter_RGB2YUV_stop: no arguments are required */

static int
filter_RGB2YUV_stop(void *arg) {
	int iid;
	if(filter_started == 0)
		return 0;
	filter_started = 0;
	for(iid = 0; iid < video_source_channels(); iid++) {
		pthread_cancel(filter_tid[iid]);
	}
	return 0;
}

ga_module_t *
module_load() {
	static ga_module_t m;
	bzero(&m, sizeof(m));
	m.type = GA_MODULE_TYPE_FILTER;
	m.name = strdup("filter-RGB2YUV");
	m.init = filter_RGB2YUV_init;
	m.start = filter_RGB2YUV_start;
	m.stop = filter_RGB2YUV_stop;
	m.deinit = filter_RGB2YUV_deinit;
	//m.threadproc = filter_RGB2YUV_threadproc;
	return &m;
}

