/*
 * libmm-imgp-gstcs
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: YoungHun Kim <yh8004.kim@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */


#include <glib.h>
#include <gst/gst.h>
#include <gst/gstbuffer.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappbuffer.h>
#include <gst/app/gstappsink.h>
#ifdef __cplusplus
        extern "C" {
#endif
#include "mm_util_gstcs.h"
#include "mm_log.h"

#define buf_size 9

typedef struct _image_format
{
	char format_label[buf_size]; //I420, AYUV, RGB888, BGRA8888
	char colorspace[buf_size]; // yuv, rgb, RGBA
	int width;
	int height;
	int stride;
	int elevation;
	int blocksize;
	GstCaps* caps;
} image_format;	

typedef struct _gstreamer
{
	GMainLoop *loop;
	GstElement *pipeline;
	GstElement *appsrc;
	GstElement *colorspace; 
	GstElement *videoscale;
	GstElement *videoflip;
	GstElement *appsink; 
	GstBuffer *output_buffer;
} gstreamer;
