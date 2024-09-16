/*
 * Copyright (c) 2013-2015 Chun-Ying Huang
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

#ifndef __GA_SERVERLESS_DPIPE_H__
#define	__GA_SERVERLESS_DPIPE_H__

/**
 * @file 
 * dpipe header: pipe for delivering discrete frames
 */

#include <stdio.h>
#include <pthread.h>
#include <sw/redis++/redis++.h>

#include "ga-common.h"
#include "vsource.h"

typedef struct serverless_dpipe_buffer_s {
	struct vsource_frame_s* pointer;
	struct serverless_dpipe_buffer_s* next;
} serverless_dpipe_buffer_t;

typedef struct serverless_dpipe_s {
	const char *name;
	int nframe;
	int channel_id;
} serverless_dpipe_t;

EXPORT serverless_dpipe_t *	serverless_dpipe_create(int id, const char *name, int nframe, int maxframesize);
EXPORT serverless_dpipe_t *	serverless_dpipe_lookup(const char *name);
EXPORT int		serverless_dpipe_destroy(serverless_dpipe_t *dpipe);
EXPORT serverless_dpipe_buffer_t *	serverless_dpipe_get(serverless_dpipe_t *dpipe);
EXPORT void		serverless_dpipe_put(serverless_dpipe_t *dpipe, serverless_dpipe_buffer_t *buffer);
EXPORT serverless_dpipe_buffer_t *	serverless_dpipe_load(serverless_dpipe_t *dpipe, const struct timespec *abstime);
EXPORT void		serverless_dpipe_store(serverless_dpipe_t *dpipe, serverless_dpipe_buffer_t *buffer);
EXPORT void serialize(serverless_dpipe_buffer_t& data, std::string& buffer);
EXPORT void deserialize(serverless_dpipe_buffer_t& data, std::string& buffer);

#endif	/* __GA_DPIPE_H__ */
