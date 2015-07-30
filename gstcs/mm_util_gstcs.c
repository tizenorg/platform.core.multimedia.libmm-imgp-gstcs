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
#include "mm_util_gstcs_internal.h"
#include <mm_debug.h>
#include <gst/check/gstcheck.h>
#include <gst/video/video-format.h>
#include <mm_error.h>
#define MM_UTIL_ROUND_UP_2(num) (((num)+1)&~1)
#define MM_UTIL_ROUND_UP_4(num) (((num)+3)&~3)
#define MM_UTIL_ROUND_UP_8(num) (((num)+7)&~7)
#define MM_UTIL_ROUND_UP_16(num) (((num)+15)&~15)

#define MAX_GET_STATE_COUNT 2000

#define setup_image_size_I420(width, height) { \
	int size=0; \
	size = (MM_UTIL_ROUND_UP_4 (width) * MM_UTIL_ROUND_UP_2 (height) + MM_UTIL_ROUND_UP_8 (width) * MM_UTIL_ROUND_UP_2 (height) /2); \
	return size; \
}

#define setup_image_size_Y42B(width, height) { \
	int size=0; \
	size = (MM_UTIL_ROUND_UP_4 (width) * height + MM_UTIL_ROUND_UP_8 (width) * height); \
	return size; \
}

#define setup_image_size_Y444(width, height) { \
	int size=0; \
	size = (MM_UTIL_ROUND_UP_4 (width) * height * 3); \
	return size; \
}

#define setup_image_size_UYVY(width, height) { \
	int size=0; \
	size = (MM_UTIL_ROUND_UP_2 (width) * 2 * height); \
	return size; \
}

#define setup_image_size_YUYV(width, height) { \
	int size=0; \
	size = (MM_UTIL_ROUND_UP_2 (width) * 2 * height); \
	return size; \
}

#define setup_image_size_YV12(width, height) { \
	int size=0; \
	size = (MM_UTIL_ROUND_UP_4 (width) * MM_UTIL_ROUND_UP_2 (height)+ MM_UTIL_ROUND_UP_8 (width) * MM_UTIL_ROUND_UP_2 (height) / 2); \
	return size; \
}

#define setup_image_size_NV12(width, height) { \
	int size=0; \
	size = (MM_UTIL_ROUND_UP_4 (width) * MM_UTIL_ROUND_UP_2 (height) *1.5); \
	return size; \
}

#define setup_image_size_RGB565(width, height) { \
	int size=0; \
	size = (MM_UTIL_ROUND_UP_4 (width * 2) *height); \
	return size; \
}

#define setup_image_size_RGB888(width, height) { \
	int size=0; \
	size = (MM_UTIL_ROUND_UP_4 (width*3) * height); \
	return size; \
}

#define setup_image_size_BGR888(width, height) { \
	int size=0; \
	size = (MM_UTIL_ROUND_UP_4 (width*3) * height); \
	return size; \
}

static void
_mm_sink_sample (GstElement * appsink, gpointer user_data)
{
	GstBuffer *_buf=NULL;
	GstSample *_sample = NULL;
	gstreamer_s * pGstreamer_s = (gstreamer_s*) user_data;
	_sample = gst_app_sink_pull_sample((GstAppSink*)appsink);
	_buf = gst_sample_get_buffer(_sample);

	pGstreamer_s->output_buffer = _buf;

	if(pGstreamer_s->output_buffer != NULL) {
		GstMapInfo mapinfo = GST_MAP_INFO_INIT;
		gst_buffer_map(pGstreamer_s->output_buffer, &mapinfo, GST_MAP_READ);
		debug_log("Create Output Buffer: GST_BUFFER_DATA: %p\t GST_BUFFER_SIZE: %d", mapinfo.data, mapinfo.size);
		gst_buffer_unmap(pGstreamer_s->output_buffer, &mapinfo);
	} else {
		debug_error("ERROR -Input Prepare Buffer! Check createoutput buffer function");
	}

	pGstreamer_s->outbuf_available = TRUE;

	gst_buffer_ref (pGstreamer_s->output_buffer); /* when you want to avoid flushing */
	gst_sample_unref(_sample);
}

static int
_mm_create_pipeline( gstreamer_s* pGstreamer_s)
{
	int ret = MM_ERROR_NONE;
	pGstreamer_s->pipeline= gst_pipeline_new ("videoconvert");
	if (!pGstreamer_s->pipeline) {
		debug_error("pipeline could not be created. Exiting.\n");
		ret = MM_ERROR_IMAGE_INVALID_VALUE;
	}
	pGstreamer_s->appsrc= gst_element_factory_make("appsrc","appsrc");
	if (!pGstreamer_s->appsrc) {
		debug_error("appsrc could not be created. Exiting.\n");
		ret = MM_ERROR_IMAGE_INVALID_VALUE;
	}
	pGstreamer_s->colorspace=gst_element_factory_make("videoconvert","colorconverter");
	if (!pGstreamer_s->colorspace) {
		debug_error("colorspace could not be created. Exiting.\n");
		ret = MM_ERROR_IMAGE_INVALID_VALUE;
	}
	pGstreamer_s->videoscale=gst_element_factory_make("videoscale", "scale");
	if (!pGstreamer_s->videoscale) {
		debug_error("videoscale could not be created. Exiting.\n");
		ret = MM_ERROR_IMAGE_INVALID_VALUE;
	}
	pGstreamer_s->videoflip=gst_element_factory_make("videoflip", "flip");
	if (!pGstreamer_s->videoflip) {
		debug_error("videoflip could not be created. Exiting.\n");
		ret = MM_ERROR_IMAGE_INVALID_VALUE;
	}
	pGstreamer_s->appsink=gst_element_factory_make("appsink","appsink");
	if (!pGstreamer_s->appsink) {
		debug_error("appsink could not be created. Exiting.\n");
		ret = MM_ERROR_IMAGE_INVALID_VALUE;
	}
	pGstreamer_s->outbuf_available = FALSE;

	return ret;
}

static gboolean
_mm_check_resize_format(int src_width, int src_height, int dst_width, int dst_height)
{
	gboolean _bool=FALSE;

	if( (src_width != dst_width) || (src_height != dst_height) ) {
		_bool = TRUE;
	}

	return _bool;
}

static gboolean
_mm_check_rotate_format(int GstVideoFlipMethod)
{
	gboolean _bool=FALSE;

	if((GstVideoFlipMethod >= 1) && ( GstVideoFlipMethod<= 7)) {
		_bool = TRUE;
	}

	return _bool;
}

static gboolean
_mm_check_resize_format_label(char* __format_label)
{
	gboolean _bool = FALSE;

	if(strcmp(__format_label, "AYUV") == 0
		|| strcmp(__format_label, "UYVY") == 0 ||strcmp(__format_label, "Y800") == 0 || strcmp(__format_label, "I420") == 0 || strcmp(__format_label, "YV12") == 0
		|| strcmp(__format_label, "RGB888") == 0 || strcmp(__format_label, "RGB565") == 0 || strcmp(__format_label, "BGR888") == 0 || strcmp(__format_label, "RGBA8888") == 0
		|| strcmp(__format_label, "ARGB8888") == 0 ||strcmp(__format_label, "BGRA8888") == 0 ||strcmp(__format_label, "ABGR8888") == 0 ||strcmp(__format_label, "RGBX") == 0
		||strcmp(__format_label, "XRGB") == 0 ||strcmp(__format_label, "BGRX") == 0 ||strcmp(__format_label, "XBGR") == 0 ||strcmp(__format_label, "Y444") == 0
		||strcmp(__format_label, "Y42B") == 0 ||strcmp(__format_label, "YUY2") == 0 ||strcmp(__format_label, "YUYV") == 0 ||strcmp(__format_label, "UYVY") == 0
		||strcmp(__format_label, "Y41B") == 0 ||strcmp(__format_label, "Y16") == 0 ||strcmp(__format_label, "Y800") == 0 ||strcmp(__format_label, "Y8") == 0
		||strcmp(__format_label, "GREY") == 0 ||strcmp(__format_label, "AY64") == 0 || strcmp(__format_label, "YUV422") == 0) {

		_bool=TRUE;
	}

	return _bool;
}

static gboolean
_mm_check_rotate_format_label(const char* __format_label)
{
	gboolean _bool = FALSE;

	if(strcmp(__format_label, "I420") == 0 ||strcmp(__format_label, "YV12") == 0 || strcmp(__format_label, "IYUV")
		|| strcmp(__format_label, "RGB888") ||strcmp(__format_label, "BGR888") == 0 ||strcmp(__format_label, "RGBA8888") == 0
		|| strcmp(__format_label, "ARGB8888") == 0 ||strcmp(__format_label, "BGRA8888") == 0 ||strcmp(__format_label, "ABGR8888") == 0 ) {
		_bool=TRUE;
	}

	return _bool;
}

static void
_mm_check_caps_format(GstCaps* caps)
{
	GstStructure *caps_structure = gst_caps_get_structure (caps, 0);
	const gchar* formatInfo = gst_structure_get_string (caps_structure, "format");
	debug_log("[%d] caps: %s", GST_IS_CAPS(caps), formatInfo);
}

static void
_mm_link_pipeline(gstreamer_s* pGstreamer_s, image_format_s* input_format, image_format_s* output_format, int value)
{
	/* set property */
	gst_app_src_set_caps(GST_APP_SRC(pGstreamer_s->appsrc), input_format->caps); /*g_object_set(pGstreamer_s->appsrc, "caps", input_format->caps, NULL);*/
	gst_app_sink_set_caps(GST_APP_SINK(pGstreamer_s->appsink), output_format->caps); /*g_object_set(pGstreamer_s->appsink, "caps", output_format->caps, NULL); */
	gst_bin_add_many(GST_BIN(pGstreamer_s->pipeline), pGstreamer_s->appsrc, pGstreamer_s->colorspace, pGstreamer_s->videoscale, pGstreamer_s->videoflip, pGstreamer_s->appsink, NULL);
	if(!gst_element_link_many(pGstreamer_s->appsrc, pGstreamer_s->colorspace, pGstreamer_s->videoscale, pGstreamer_s->videoflip, pGstreamer_s->appsink, NULL)) {
		debug_error("Fail to link pipeline");
	} else {
		debug_log("Success to link pipeline");
	}
	
	g_object_set (G_OBJECT (pGstreamer_s->appsrc), "stream-type", 0, "format", GST_FORMAT_TIME, NULL);
	g_object_set(pGstreamer_s->appsrc, "num-buffers", 1, NULL);
	g_object_set(pGstreamer_s->appsrc, "is-live", TRUE, NULL); /* add because of gstreamer_s time issue */

	g_object_set(pGstreamer_s->videoflip, "method", value, NULL ); /* GST_VIDEO_FLIP_METHOD_IDENTITY (0): none- Identity (no rotation) (1): clockwise - Rotate clockwise 90 degrees (2): rotate-180 - Rotate 180 degrees (3): counterclockwise - Rotate counter-clockwise 90 degrees (4): horizontal-flip - Flip horizontally (5): vertical-flip - Flip vertically (6): upper-left-diagonal - Flip across upper left/lower right diagonal (7): upper-right-diagonal - Flip across upper right/lower left diagonal */

	g_object_set(pGstreamer_s->appsink, "drop", TRUE, NULL);
	g_object_set(pGstreamer_s->appsink, "emit-signals", TRUE, "sync", FALSE, NULL);
}

static void
_mm_set_image_format_s_capabilities(image_format_s* __format) /*_format_label: I420 _colorsapace: YUV */
{
	char _format_name[sizeof(GST_VIDEO_FORMATS_ALL)] = {'\0'};
	GstVideoFormat videoFormat;
	int _bpp=0; int _depth=0; int _red_mask=0; int _green_mask=0; int _blue_mask=0; int _alpha_mask=0; int _endianness=0;

	if(__format == NULL) {
		debug_error("Image format is NULL\n");
		return;
	}

	debug_log("colorspace: %s(%d)\n", __format->colorspace, strlen(__format->colorspace));

	if(strcmp(__format->colorspace,"YUV") == 0) {
		if(strcmp(__format->format_label,"I420") == 0
		|| strcmp(__format->format_label,"Y42B") == 0
		|| strcmp(__format->format_label,"Y444") == 0
		|| strcmp(__format->format_label,"YV12") == 0
		|| strcmp(__format->format_label,"NV12") == 0
		|| strcmp(__format->format_label,"UYVY") == 0) {
			strncpy(_format_name, __format->format_label, sizeof(GST_VIDEO_FORMATS_ALL)-1);
		}else if(strcmp(__format->format_label,"YUYV") == 0) {
			strncpy(_format_name, "YVYU", sizeof(GST_VIDEO_FORMATS_ALL)-1);
		}

		__format->caps = gst_caps_new_simple ("video/x-raw",
			"format", G_TYPE_STRING, _format_name,
			"framerate", GST_TYPE_FRACTION, 25, 1,
			"pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
			"width", G_TYPE_INT, __format->width,
			"height", G_TYPE_INT, __format->height,
			"framerate", GST_TYPE_FRACTION, 1, 1,
			NULL);
	}

	else if(strcmp(__format->colorspace,"RGB") ==0 || strcmp(__format->colorspace,"BGRX") ==0) {
		if(strcmp(__format->format_label,"RGB888") == 0) {
			_bpp=24; _depth=24; _red_mask=16711680; _green_mask=65280; _blue_mask=255; _endianness=4321;
		}else if(strcmp(__format->format_label,"BGR888") == 0) {
			_bpp=24; _depth=24; _red_mask=255; _green_mask=65280; _blue_mask=16711680; _endianness=4321;
		}else if(strcmp(__format->format_label,"RGB565") == 0) {
			_bpp=16; _depth=16; _red_mask=63488; _green_mask=2016; _blue_mask=31; _endianness=1234;
		}else if( (strcmp(__format->format_label, "BGRX") == 0)) {
			_bpp=32; _depth=24; _red_mask=65280; _green_mask=16711680; _blue_mask=-16777216; _endianness=4321;
		}

		videoFormat = gst_video_format_from_masks(_depth, _bpp, _endianness, _red_mask, _green_mask, _blue_mask, 0);
		debug_log("Chosen video format: %s", gst_video_format_to_string(videoFormat));

		__format->caps = gst_caps_new_simple ("video/x-raw",
			"format", G_TYPE_STRING, gst_video_format_to_string(videoFormat),
			"width", G_TYPE_INT, __format->width,
			"height", G_TYPE_INT, __format->height,
			"framerate", GST_TYPE_FRACTION, 1, 1, NULL);
	}

	else if(strcmp(__format->colorspace,"RGBA") ==0) {
		if(strcmp(__format->format_label,"ARGB8888") == 0) { /*[Low Arrary Address] ARGBARGB... [High Array Address]*/
			debug_log("ARGB8888");
			_bpp=32; _depth=32; _red_mask=16711680; _green_mask=65280; _blue_mask=255; _alpha_mask=-16777216; _endianness=4321;
		}else if(strcmp(__format->format_label,"BGRA8888") == 0) { /*[Low Arrary Address] BGRABGRA...[High Array Address]*/
			debug_log("BGRA8888");
			_bpp=32; _depth=32; _red_mask=65280; _green_mask=16711680; _blue_mask=-16777216; _alpha_mask=255; _endianness=4321;
		}else if(strcmp(__format->format_label,"RGBA8888") == 0) { /*[Low Arrary Address] RGBARGBA...[High Array Address]*/
			debug_log("RGBA8888");
			_bpp=32; _depth=32; _red_mask=-16777216; _green_mask=16711680; _blue_mask=65280; _alpha_mask=255; _endianness=4321;
		}else if(strcmp(__format->format_label,"ABGR8888") == 0) { /*[Low Arrary Address] ABGRABGR...[High Array Address]*/
			debug_log("ABGR8888");
			_bpp=32; _depth=32; _red_mask=255; _green_mask=65280; _blue_mask=16711680; _alpha_mask=-16777216; _endianness=4321;
		}else {
			debug_error("***Wrong format cs type***\n");
		}

		videoFormat = gst_video_format_from_masks(_depth, _bpp, _endianness, _red_mask, _green_mask, _blue_mask, _alpha_mask);
		debug_log("Chosen video format: %s", gst_video_format_to_string(videoFormat));

		__format->caps = gst_caps_new_simple ("video/x-raw",
			"format", G_TYPE_STRING, gst_video_format_to_string(videoFormat),
			"width", G_TYPE_INT, __format->width,
			"height", G_TYPE_INT, __format->height,
			"framerate", GST_TYPE_FRACTION, 1, 1, NULL);
	}

	if(__format->caps) {
		debug_log("###__format->caps is not NULL###, %p", __format->caps);
		_mm_check_caps_format(__format->caps);
	}else {
		debug_error("__format->caps is NULL");
	}
}

static void
_mm_set_image_colorspace( image_format_s* __format)
{
	debug_log("format_label: %s\n", __format->format_label);

	__format->colorspace = (char*)malloc(sizeof(char) * IMAGE_FORMAT_LABEL_BUFFER_SIZE);
	if (__format->colorspace == NULL) {
		debug_error("memory allocation failed");
		return;
	}
	memset(__format->colorspace, 0, IMAGE_FORMAT_LABEL_BUFFER_SIZE);
	if( (strcmp(__format->format_label, "I420") == 0) ||(strcmp(__format->format_label, "Y42B") == 0) || (strcmp(__format->format_label, "Y444") == 0)
		|| (strcmp(__format->format_label, "YV12") == 0) ||(strcmp(__format->format_label, "NV12") == 0) ||(strcmp(__format->format_label, "UYVY") == 0) ||(strcmp(__format->format_label, "YUYV") == 0)) {
		strncpy(__format->colorspace, "YUV", IMAGE_FORMAT_LABEL_BUFFER_SIZE-1);
	}else if( (strcmp(__format->format_label, "RGB888") == 0) ||(strcmp(__format->format_label, "BGR888") == 0) ||(strcmp(__format->format_label, "RGB565") == 0)) {
		strncpy(__format->colorspace, "RGB", IMAGE_FORMAT_LABEL_BUFFER_SIZE-1);
	}else if( (strcmp(__format->format_label, "ARGB8888") == 0) || (strcmp(__format->format_label, "BGRA8888") == 0) ||(strcmp(__format->format_label, "RGBA8888") == 0)	|| (strcmp(__format->format_label, "ABGR8888") == 0)) {
		strncpy(__format->colorspace, "RGBA",IMAGE_FORMAT_LABEL_BUFFER_SIZE-1);
	}else if( (strcmp(__format->format_label, "BGRX") == 0)) {
		strncpy(__format->colorspace, "BGRX", IMAGE_FORMAT_LABEL_BUFFER_SIZE-1);
	}else {
		debug_error("Check your colorspace format label");
		GSTCS_FREE(__format->colorspace);
	}
}

static image_format_s*
_mm_set_input_image_format_s_struct(imgp_info_s* pImgp_info) /* char* __format_label, int __width, int __height) */
{
	image_format_s* __format = NULL;

	__format=(image_format_s*)malloc(sizeof(image_format_s));
	if (__format == NULL) {
		debug_error("memory allocation failed");
		return NULL;
	}
	memset(__format, 0, sizeof(image_format_s));

	__format->format_label = (char *)malloc(sizeof(char) * IMAGE_FORMAT_LABEL_BUFFER_SIZE);
	if (__format->format_label == NULL) {
		debug_error("memory allocation failed");
		GSTCS_FREE(__format);
		return NULL;
	}
	memset(__format->format_label, 0, IMAGE_FORMAT_LABEL_BUFFER_SIZE);
	strncpy(__format->format_label, pImgp_info->input_format_label, strlen(pImgp_info->input_format_label));
	debug_log("input_format_label: %s\n", pImgp_info->input_format_label);
	_mm_set_image_colorspace(__format);

	__format->width=pImgp_info->src_width;
	__format->height=pImgp_info->src_height;

	__format->blocksize = mm_setup_image_size(pImgp_info->input_format_label, pImgp_info->src_width, pImgp_info->src_height);
	debug_log("blocksize: %d\n", __format->blocksize);
	_mm_set_image_format_s_capabilities(__format);

	return __format;
}

static void
_mm_round_up_output_image_widh_height(image_format_s* pFormat)
{
	if(strcmp(pFormat->colorspace,"YUV") ==0) {
		pFormat->stride=MM_UTIL_ROUND_UP_8(pFormat->width);
		pFormat->elevation=MM_UTIL_ROUND_UP_2(pFormat->height);
	} else if(strcmp(pFormat->colorspace, "RGB") ==0) {
		pFormat->stride=MM_UTIL_ROUND_UP_4(pFormat->width);
		pFormat->elevation=MM_UTIL_ROUND_UP_2(pFormat->height);
	}
}

static image_format_s*
_mm_set_output_image_format_s_struct(imgp_info_s* pImgp_info)
{
	image_format_s* __format = NULL;

	__format=(image_format_s*)malloc(sizeof(image_format_s));
	if (__format == NULL) {
		debug_error("memory allocation failed");
		return NULL;
	}
	memset(__format, 0, sizeof(image_format_s));

	__format->format_label = (char *)malloc(sizeof(char) * IMAGE_FORMAT_LABEL_BUFFER_SIZE);
	if (__format->format_label == NULL) {
		debug_error("memory allocation failed");
		GSTCS_FREE(__format);
		return NULL;
	}
	memset(__format->format_label, 0, IMAGE_FORMAT_LABEL_BUFFER_SIZE);
	strncpy(__format->format_label, pImgp_info->output_format_label, strlen(pImgp_info->output_format_label));
	_mm_set_image_colorspace(__format);

	__format->width=pImgp_info->dst_width;
	__format->height=pImgp_info->dst_height;
	_mm_round_up_output_image_widh_height(__format);

	__format->blocksize = mm_setup_image_size(pImgp_info->output_format_label, pImgp_info->dst_width, pImgp_info->dst_height);
	debug_log("output_format_label: %s", pImgp_info->output_format_label);
	_mm_set_image_format_s_capabilities(__format);
	return __format;
}

static int
_mm_push_buffer_into_pipeline(imgp_info_s* pImgp_info, unsigned char *src, gstreamer_s * pGstreamer_s)
{
	int ret = MM_ERROR_NONE;

	if(pGstreamer_s->pipeline == NULL) {
		debug_error("pipeline is NULL\n");
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}

	gsize data_size = mm_setup_image_size(pImgp_info->input_format_label, pImgp_info->src_width, pImgp_info->src_height);
	GstBuffer* gst_buf = gst_buffer_new_wrapped_full(GST_MEMORY_FLAG_READONLY, src, data_size, 0, data_size, NULL, NULL);

	if(gst_buf==NULL) {
		debug_error("buffer is NULL\n");
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}

	gst_app_src_push_buffer (GST_APP_SRC (pGstreamer_s->appsrc), gst_buf); /* push buffer to pipeline */
	return ret;
}


static int
_mm_imgp_gstcs_processing( gstreamer_s* pGstreamer_s, unsigned char *src, unsigned char *dst, image_format_s* input_format, image_format_s* output_format, imgp_info_s* pImgp_info)
{
	GstBus *bus = NULL;
	GstStateChangeReturn ret_state;
	int ret = MM_ERROR_NONE;
	int i = 0;

	if(src== NULL || dst == NULL) {
		debug_error("src || dst is NULL");
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}

	/*create pipeline*/
	ret = _mm_create_pipeline(pGstreamer_s);
	if(ret != MM_ERROR_NONE) {
		debug_error("ERROR - mm_create_pipeline ");
	}


	/* Make appsink emit the "new-preroll" and "new-sample" signals. This option is by default disabled because signal emission is expensive and unneeded when the application prefers to operate in pull mode. */
	gst_app_sink_set_emit_signals ((GstAppSink*)pGstreamer_s->appsink, TRUE);

	bus = gst_pipeline_get_bus (GST_PIPELINE (pGstreamer_s->pipeline)); /* GST_PIPELINE (pipeline)); */
	gst_object_unref(bus);

	debug_log("Start mm_push_buffer_into_pipeline");
	ret = _mm_push_buffer_into_pipeline(pImgp_info, src, pGstreamer_s);
	if(ret != MM_ERROR_NONE) {
		debug_error("ERROR - mm_push_buffer_into_pipeline ");
	}
	debug_log("End mm_push_buffer_into_pipeline");

	/*link pipeline*/
	debug_log("Start mm_link_pipeline");
	_mm_link_pipeline( pGstreamer_s, input_format, output_format, pImgp_info->angle);
	debug_log("End mm_link_pipeline");

	/* Conecting to the new-sample signal emited by the appsink*/
	debug_log("Start G_CALLBACK (_mm_sink_sample)");
	g_signal_connect (pGstreamer_s->appsink, "new-sample", G_CALLBACK (_mm_sink_sample), pGstreamer_s);
	debug_log("End G_CALLBACK (_mm_sink_sample)");

	/* GST_STATE_PLAYING*/
	debug_log("Start GST_STATE_PLAYING");
	ret_state = gst_element_set_state (pGstreamer_s->pipeline, GST_STATE_PLAYING);
	debug_log("End GST_STATE_PLAYING ret_state: %d", ret_state);

	do {
		ret_state = gst_element_get_state (pGstreamer_s->pipeline, NULL, NULL, 10*GST_MSECOND);
		debug_error("gst_element_get_state ret_state: %d [%d, %d]", ret_state, ++i, pGstreamer_s->outbuf_available);
		 if ((ret_state == GST_STATE_CHANGE_SUCCESS) && (pGstreamer_s->outbuf_available)) {
		 	debug_log("available output buffer & success state chnage");
		 	break;
		 }
	} while (i < MAX_GET_STATE_COUNT);

	if (i == MAX_GET_STATE_COUNT) {
		debug_error("Fail GST_STATE_CHANGE");
		gst_object_unref (pGstreamer_s->pipeline);
		g_free (pGstreamer_s);

		return MM_ERROR_IMAGE_INVALID_VALUE;
	}

	debug_log("Sucess GST_STATE_CHANGE");

	/* error */
	if (ret_state == GST_STATE_CHANGE_FAILURE) {
		debug_error("GST_STATE_CHANGE_FAILURE");
		gst_object_unref (pGstreamer_s->pipeline);
		g_free (pGstreamer_s);

		return MM_ERROR_IMAGE_INVALID_VALUE;
	}else {
		debug_log("Set GST_STATE_NULL");

		/*GST_STATE_NULL*/
		gst_element_set_state (pGstreamer_s->pipeline, GST_STATE_NULL);
		debug_log("End GST_STATE_NULL");

		debug_log("###pGstreamer_s->output_buffer### : %p", pGstreamer_s->output_buffer);

		ret_state = gst_element_get_state (pGstreamer_s->pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

		if(ret_state == GST_STATE_CHANGE_SUCCESS) {
			debug_log("GST_STATE_NULL ret_state = %d (GST_STATE_CHANGE_SUCCESS)\n", ret_state);
		}else if( ret_state == GST_STATE_CHANGE_ASYNC) {
			debug_log("GST_STATE_NULL ret_state = %d (GST_STATE_CHANGE_ASYNC)\n", ret_state);
		}

		debug_log("Success gst_element_get_state\n");

		if (ret_state == GST_STATE_CHANGE_FAILURE) {
			debug_error("GST_STATE_CHANGE_FAILURE");
		}else {
			if(pGstreamer_s->output_buffer != NULL) {
				GstMapInfo mapinfo = GST_MAP_INFO_INIT;
				gst_buffer_map(pGstreamer_s->output_buffer, &mapinfo, GST_MAP_READ);
				int buffer_size = mapinfo.size;
				debug_log("buffer size: %d\n", buffer_size);
				if( buffer_size != mm_setup_image_size(pImgp_info->output_format_label, pImgp_info->output_stride, pImgp_info->output_elevation)) {
					debug_log("Buffer size is different stride:%d elevation: %d\n", pImgp_info->output_stride, pImgp_info->output_elevation);
				}
				debug_log("pGstreamer_s->output_buffer: 0x%2x\n", pGstreamer_s->output_buffer);
				memcpy(dst, mapinfo.data, buffer_size);
				gst_buffer_unmap(pGstreamer_s->output_buffer, &mapinfo);
			}else {
				debug_log("pGstreamer_s->output_buffer is NULL");
			}
		}
		gst_buffer_unref (pGstreamer_s->output_buffer);
		debug_log("unref output buffer");
		gst_object_unref (pGstreamer_s->pipeline);
		pGstreamer_s->output_buffer = NULL;
		g_free (pGstreamer_s);

		debug_log("End gstreamer processing");
	}
	debug_log("dst: %p", dst);
	return ret;
}


static int
mm_setup_image_size(const char* _format_label, int width, int height)
{
	int size=0;

	if(strcmp(_format_label, "I420") == 0) {
		setup_image_size_I420(width, height); /*width * height *1.5; */
	}else if(strcmp(_format_label, "Y42B") == 0) {
		setup_image_size_Y42B(width, height); /*width * height *2; */
	}else if(strcmp(_format_label, "YUV422") == 0) {
		setup_image_size_Y42B(width, height); /*width * height *2; */
	}else if(strcmp(_format_label, "Y444") == 0) {
		setup_image_size_Y444(width, height); /* width * height *3; */
	}else if(strcmp(_format_label, "YV12") == 0) {
		setup_image_size_YV12(width, height); /* width * height *1; */
	}else if(strcmp(_format_label, "NV12") == 0) {
		setup_image_size_NV12(width, height); /* width * height *1.5; */
	}else if(strcmp(_format_label, "RGB565") == 0) {
		setup_image_size_RGB565(width, height); /* width * height *2; */
	}else if(strcmp(_format_label, "RGB888") == 0) {
		setup_image_size_RGB888(width, height); /* width * height *3; */
	}else if(strcmp(_format_label, "BGR888") == 0) {
		setup_image_size_BGR888(width, height); /* width * height *3; */
	}else if(strcmp(_format_label, "UYVY") == 0) {
		setup_image_size_UYVY(width, height); /* width * height *2; */
	}else if(strcmp(_format_label, "YUYV") == 0) {
		setup_image_size_YUYV(width, height); /* width * height *2; */
	}else if(strcmp(_format_label, "ARGB8888") == 0) {
		size = width * height *4; debug_log("file_size: %d\n", size);
	}else if(strcmp(_format_label, "BGRA8888") == 0) {
		size = width * height *4; debug_log("file_size: %d\n", size);
	}else if(strcmp(_format_label, "RGBA8888") == 0) {
		size = width * height *4; debug_log("file_size: %d\n", size);
	}else if(strcmp(_format_label, "ABGR8888") == 0) {
		size = width * height *4; debug_log("file_size: %d\n", size);
	}else if(strcmp(_format_label, "BGRX") == 0) {
		size = width * height *4; debug_log("file_size: %d\n", size);
	}

	return size;
}

static int
_mm_imgp_gstcs(imgp_info_s* pImgp_info, unsigned char *src, unsigned char *dst)
{
	image_format_s* input_format=NULL, *output_format=NULL;
	gstreamer_s* pGstreamer_s;
	int ret = MM_ERROR_NONE;
	static const int max_argc = 50;
	gint* argc = NULL;
	gchar** argv = NULL;

	g_type_init();
	if(pImgp_info == NULL) {
		debug_error("imgp_info_s is NULL");
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}

	if(src== NULL || dst == NULL) {
		debug_error("src || dst is NULL");
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}
	debug_log("[src %p] [dst %p]", src, dst);

	argc = malloc(sizeof(int));
	argv = malloc(sizeof(gchar*) * max_argc);

	if (!argc || !argv) {
		debug_error("argc ||argv is NULL");
		GSTCS_FREE(input_format);
		GSTCS_FREE(output_format);
		if (argc != NULL) {
			free(argc);
		}
		if (argv != NULL) {
			free(argv);
		}
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}
	memset(argv, 0, sizeof(gchar*) * max_argc);
	debug_log("memset argv");

	/* add initial */
	*argc = 1;
	argv[0] = g_strdup( "mmutil_gstcs" );
	/* check disable registry scan */
	argv[*argc] = g_strdup("--gst-disable-registry-update");
	(*argc)++;
	debug_log("--gst-disable-registry-update");

	gst_init(argc, &argv);

	pGstreamer_s = g_new0 (gstreamer_s, 1);

	GSTCS_FREE(argv);
	GSTCS_FREE(argc);

	debug_log("[input] format label : %s width: %d height: %d\t[output] format label: %s width: %d height: %d rotation vaule: %d",
	pImgp_info->input_format_label, pImgp_info->src_width, pImgp_info->src_height, pImgp_info->output_format_label, pImgp_info->dst_width, pImgp_info->dst_height, pImgp_info->angle);

	input_format= _mm_set_input_image_format_s_struct(pImgp_info);
	if (input_format == NULL) {
		debug_error("memory allocation failed");
		return MM_ERROR_IMAGE_NO_FREE_SPACE;
	}
	output_format= _mm_set_output_image_format_s_struct(pImgp_info);
	if (output_format == NULL) {
		debug_error("memory allocation failed");
		GSTCS_FREE(input_format->format_label);
		GSTCS_FREE(input_format->colorspace);
		GSTCS_FREE(input_format);
		return MM_ERROR_IMAGE_NO_FREE_SPACE;
	}

	pImgp_info->output_stride = output_format->width;
	pImgp_info->output_elevation = output_format->height;

	/* _format_label : I420, RGB888 etc*/
	debug_log("Start _mm_imgp_gstcs_processing ");
	ret =_mm_imgp_gstcs_processing(pGstreamer_s, src, dst, input_format, output_format, pImgp_info); /* input: buffer pointer for input image , input image format, input image width, input image height, output: buffer porinter for output image */

	if(ret == MM_ERROR_NONE) {
		debug_log("End _mm_imgp_gstcs_processing [dst: %p]", dst);
	}else if (ret != MM_ERROR_NONE) {
		debug_error("ERROR - _mm_imgp_gstcs_processing");
	}

	GSTCS_FREE(input_format->format_label);
	GSTCS_FREE(input_format->colorspace);
	GSTCS_FREE(input_format);

	GSTCS_FREE(output_format->format_label);
	GSTCS_FREE(output_format->colorspace);
	GSTCS_FREE(output_format);

	return ret;
}

int
mm_imgp(imgp_info_s* pImgp_info, unsigned char *src, unsigned char *dst, imgp_type_e _imgp_type)
{
	if (pImgp_info == NULL) {
		debug_error("input vaule is error");
	}
	return _mm_imgp_gstcs(pImgp_info, src, dst);
}
