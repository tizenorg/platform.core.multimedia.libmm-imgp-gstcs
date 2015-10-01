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
#include <gst/check/gstcheck.h>
#include <gst/video/video-format.h>

#define MM_UTIL_ROUND_UP_2(num) (((num)+1)&~1)
#define MM_UTIL_ROUND_UP_4(num) (((num)+3)&~3)
#define MM_UTIL_ROUND_UP_8(num) (((num)+7)&~7)
#define MM_UTIL_ROUND_UP_16(num) (((num)+15)&~15)


static GstFlowReturn
_mm_sink_sample(GstElement * appsink, gpointer user_data)
{
	GstBuffer *_buf=NULL;
	GstSample *_sample = NULL;
	gstreamer_s * pGstreamer_s = (gstreamer_s*) user_data;
	_sample = gst_app_sink_pull_sample((GstAppSink*)appsink);
	if (_sample) {
		_buf = gst_sample_get_buffer(_sample);

		pGstreamer_s->output_buffer = _buf;

		if(pGstreamer_s->output_buffer != NULL) {
			GstMapInfo mapinfo = GST_MAP_INFO_INIT;
			gst_buffer_map(pGstreamer_s->output_buffer, &mapinfo, GST_MAP_READ);
			gstcs_debug("Create Output Buffer: GST_BUFFER_DATA: %p\t GST_BUFFER_SIZE: %d", mapinfo.data, mapinfo.size);
			gst_buffer_unmap(pGstreamer_s->output_buffer, &mapinfo);
		} else {
			gstcs_error("ERROR -Input Prepare Buffer! Check createoutput buffer function");
		}
	}

	gst_buffer_ref (pGstreamer_s->output_buffer); /* when you want to avoid flushing */
	gst_sample_unref(_sample);

	return GST_FLOW_OK;
}

static gboolean
_mm_on_src_message(GstBus * bus, GstMessage * message, gpointer user_data)
{
	gstreamer_s * pGstreamer_s = (gstreamer_s*) user_data;
	switch (GST_MESSAGE_TYPE (message)) {
	case GST_MESSAGE_EOS: {
		gstcs_debug("The source got dry");
		gst_app_src_end_of_stream(GST_APP_SRC(pGstreamer_s->appsrc));
		g_main_context_pop_thread_default(pGstreamer_s->context);
		g_main_loop_quit (pGstreamer_s->loop);
		break;
		}
	case GST_MESSAGE_ERROR: {
		GError *err = NULL;
		gchar *dbg_info = NULL;

		gst_message_parse_error (message, &err, &dbg_info);
		gstcs_error ("ERROR from element %s: %s\n", GST_OBJECT_NAME (message->src), err->message);
		gstcs_error ("Debugging info: %s\n", (dbg_info) ? dbg_info : "none");
		g_error_free (err);
		g_free (dbg_info);
		g_main_context_pop_thread_default(pGstreamer_s->context);
		g_main_loop_quit (pGstreamer_s->loop);
		gstcs_debug("Quit GST_CS\n");
		break;
		}
	default:
		break;
	}
	return TRUE;
}

static int
_mm_get_byte_per_pixcel(const char *__format_label)
{
	int byte_per_pixcel = 1;
	if (strcmp(__format_label, "RGB565") == 0) {
		byte_per_pixcel = 2;
	} else if (strcmp(__format_label, "RGB888") == 0 ||
		strcmp(__format_label, "BGR888") == 0) {
		byte_per_pixcel = 3;
	} else if (strcmp(__format_label, "RGBA8888") == 0 ||
		strcmp(__format_label, "ARGB8888") == 0 ||
		strcmp(__format_label, "BGRA8888") == 0 ||
		strcmp(__format_label, "ABGR8888") == 0 ||
		strcmp(__format_label, "RGBX") == 0 ||
		strcmp(__format_label, "XRGB") == 0 ||
		strcmp(__format_label, "BGRX") == 0 ||
		strcmp(__format_label, "XBGR") == 0) {
		byte_per_pixcel = 4;
	}

	gstcs_debug("byte per pixcel : %d", byte_per_pixcel);

	return byte_per_pixcel;
}

static int
_mm_create_pipeline( gstreamer_s* pGstreamer_s)
{
	int ret = GSTCS_ERROR_NONE;
	pGstreamer_s->pipeline= gst_pipeline_new ("videoconvert");
	if (!pGstreamer_s->pipeline) {
		gstcs_error("pipeline could not be created. Exiting.\n");
		ret = GSTCS_ERROR_INVALID_PARAMETER;
	}
	pGstreamer_s->appsrc= gst_element_factory_make("appsrc","appsrc");
	if (!pGstreamer_s->appsrc) {
		gstcs_error("appsrc could not be created. Exiting.\n");
		ret = GSTCS_ERROR_INVALID_PARAMETER;
	}
	pGstreamer_s->colorspace=gst_element_factory_make("videoconvert","colorspace");
	if (!pGstreamer_s->colorspace) {
		gstcs_error("colorspace could not be created. Exiting.\n");
		ret = GSTCS_ERROR_INVALID_PARAMETER;
	}
	pGstreamer_s->videoscale=gst_element_factory_make("videoscale", "scale");
	if (!pGstreamer_s->videoscale) {
		gstcs_error("videoscale could not be created. Exiting.\n");
		ret = GSTCS_ERROR_INVALID_PARAMETER;
	}
	pGstreamer_s->videoflip=gst_element_factory_make("videoflip", "flip");
	if (!pGstreamer_s->videoflip) {
		gstcs_error("videoflip could not be created. Exiting.\n");
		ret = GSTCS_ERROR_INVALID_PARAMETER;
	}
	pGstreamer_s->appsink=gst_element_factory_make("appsink","appsink");
	if (!pGstreamer_s->appsink) {
		gstcs_error("appsink could not be created. Exiting.\n");
		ret = GSTCS_ERROR_INVALID_PARAMETER;
	}
	return ret;
}

static void _mm_destroy_notify(gpointer data) {
	unsigned char *_data = (unsigned char *)data;
	if (_data != NULL) {
		free(_data);
		_data = NULL;
	}
}

static void
_mm_check_caps_format(GstCaps* caps)
{
	GstStructure *caps_structure = gst_caps_get_structure (caps, 0);
	const gchar* formatInfo = gst_structure_get_string (caps_structure, "format");
	gstcs_debug("[%d] caps: %s", GST_IS_CAPS(caps), formatInfo);
}

static void
_mm_link_pipeline(gstreamer_s* pGstreamer_s, image_format_s* input_format, image_format_s* output_format, int value)
{
	/* set property */
	gst_app_src_set_caps(GST_APP_SRC(pGstreamer_s->appsrc), input_format->caps); /*g_object_set(pGstreamer_s->appsrc, "caps", input_format->caps, NULL);*/
	gst_app_sink_set_caps(GST_APP_SINK(pGstreamer_s->appsink), output_format->caps); /*g_object_set(pGstreamer_s->appsink, "caps", output_format->caps, NULL); */
	gst_bin_add_many(GST_BIN(pGstreamer_s->pipeline), pGstreamer_s->appsrc, pGstreamer_s->colorspace, pGstreamer_s->videoscale, pGstreamer_s->videoflip, pGstreamer_s->appsink, NULL);
	if(!gst_element_link_many(pGstreamer_s->appsrc, pGstreamer_s->colorspace, pGstreamer_s->videoscale, pGstreamer_s->videoflip, pGstreamer_s->appsink, NULL)) {
		gstcs_error("Fail to link pipeline");
	} else {
		gstcs_debug("Success to link pipeline");
	}

	g_object_set (G_OBJECT (pGstreamer_s->appsrc), "stream-type", 0, "format", GST_FORMAT_TIME, NULL);
	g_object_set(pGstreamer_s->appsrc, "num-buffers", 1, NULL);
	g_object_set(pGstreamer_s->appsrc, "is-live", TRUE, NULL); /* add because of gstreamer_s time issue */

	g_object_set(pGstreamer_s->videoflip, "method", value, NULL ); /* GST_VIDEO_FLIP_METHOD_IDENTITY (0): none- Identity (no rotation) (1): clockwise - Rotate clockwise 90 degrees (2): rotate-180 - Rotate 180 degrees (3): counterclockwise - Rotate counter-clockwise 90 degrees (4): horizontal-flip - Flip horizontally (5): vertical-flip - Flip vertically (6): upper-left-diagonal - Flip across upper left/lower right diagonal (7): upper-right-diagonal - Flip across upper right/lower left diagonal */

	/*g_object_set(pGstreamer_s->appsink, "drop", TRUE, NULL);*/
	g_object_set(pGstreamer_s->appsink, "emit-signals", TRUE, "sync", FALSE, NULL);
}

static void
_mm_set_image_input_format_s_capabilities(image_format_s* __format) /*_format_label: I420 _colorsapace: YUV */
{
	char _format_name[sizeof(GST_VIDEO_FORMATS_ALL)] = {'\0'};
	GstVideoFormat videoFormat;
	int _bpp=0; int _depth=0; int _red_mask=0; int _green_mask=0; int _blue_mask=0; int _alpha_mask=0; int _endianness=0;

	if(__format == NULL) {
		gstcs_error("Image format is NULL\n");
		return;
	}

	gstcs_debug("colorspace: %s(%d)\n", __format->colorspace, strlen(__format->colorspace));
	memset(_format_name, 0, sizeof(_format_name));

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
		gstcs_debug("Chosen video format: %s", _format_name);
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
		gstcs_debug("Chosen video format: %s", gst_video_format_to_string(videoFormat));

		__format->caps = gst_caps_new_simple ("video/x-raw",
			"format", G_TYPE_STRING, gst_video_format_to_string(videoFormat),
			"width", G_TYPE_INT, __format->stride,
			"height", G_TYPE_INT, __format->elevation,
			"framerate", GST_TYPE_FRACTION, 1, 1, NULL);
	}

	else if(strcmp(__format->colorspace,"RGBA") ==0) {
		if(strcmp(__format->format_label,"ARGB8888") == 0) { /*[Low Arrary Address] ARGBARGB... [High Array Address]*/
			gstcs_debug("ARGB8888");
			_bpp=32; _depth=32; _red_mask=16711680; _green_mask=65280; _blue_mask=255; _alpha_mask=-16777216; _endianness=4321;
		}else if(strcmp(__format->format_label,"BGRA8888") == 0) { /*[Low Arrary Address] BGRABGRA...[High Array Address]*/
			gstcs_debug("BGRA8888");
			_bpp=32; _depth=32; _red_mask=65280; _green_mask=16711680; _blue_mask=-16777216; _alpha_mask=255; _endianness=4321;
		}else if(strcmp(__format->format_label,"RGBA8888") == 0) { /*[Low Arrary Address] RGBARGBA...[High Array Address]*/
			gstcs_debug("RGBA8888");
			_bpp=32; _depth=32; _red_mask=-16777216; _green_mask=16711680; _blue_mask=65280; _alpha_mask=255; _endianness=4321;
		}else if(strcmp(__format->format_label,"ABGR8888") == 0) { /*[Low Arrary Address] ABGRABGR...[High Array Address]*/
			gstcs_debug("ABGR8888");
			_bpp=32; _depth=32; _red_mask=255; _green_mask=65280; _blue_mask=16711680; _alpha_mask=-16777216; _endianness=4321;
		}else {
			gstcs_error("***Wrong format cs type***\n");
		}

		videoFormat = gst_video_format_from_masks(_depth, _bpp, _endianness, _red_mask, _green_mask, _blue_mask, _alpha_mask);
		gstcs_debug("Chosen video format: %s", gst_video_format_to_string(videoFormat));

		__format->caps = gst_caps_new_simple ("video/x-raw",
			"format", G_TYPE_STRING, gst_video_format_to_string(videoFormat),
			"width", G_TYPE_INT, __format->width,
			"height", G_TYPE_INT, __format->elevation,
			"framerate", GST_TYPE_FRACTION, 1, 1, NULL);
	}

	if(__format->caps) {
		gstcs_debug("###__format->caps is not NULL###, %p", __format->caps);
		_mm_check_caps_format(__format->caps);
	}else {
		gstcs_error("__format->caps is NULL");
	}
}

static void
_mm_set_image_output_format_s_capabilities(image_format_s* __format) /*_format_label: I420 _colorsapace: YUV */
{
	char _format_name[sizeof(GST_VIDEO_FORMATS_ALL)] = {'\0'};
	GstVideoFormat videoFormat;
	int _bpp=0; int _depth=0; int _red_mask=0; int _green_mask=0; int _blue_mask=0; int _alpha_mask=0; int _endianness=0;

	if(__format == NULL) {
		gstcs_error("Image format is NULL\n");
		return;
	}

	gstcs_debug("colorspace: %s(%d), w: %d, h: %d\n", __format->colorspace, strlen(__format->colorspace), __format->width, __format->height);
	memset(_format_name, 0, sizeof(_format_name));

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
		gstcs_debug("Chosen video format: %s", _format_name);
		__format->caps = gst_caps_new_simple ("video/x-raw",
			"format", G_TYPE_STRING, _format_name,
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
		gstcs_debug("Chosen video format: %s", gst_video_format_to_string(videoFormat));

		__format->caps = gst_caps_new_simple ("video/x-raw",
			"format", G_TYPE_STRING, gst_video_format_to_string(videoFormat),
			"width", G_TYPE_INT, __format->width,
			"height", G_TYPE_INT, __format->height,
			"framerate", GST_TYPE_FRACTION, 1, 1, NULL);
	}

	else if(strcmp(__format->colorspace,"RGBA") ==0) {
		if(strcmp(__format->format_label,"ARGB8888") == 0) { /*[Low Arrary Address] ARGBARGB... [High Array Address]*/
			gstcs_debug("ARGB8888");
			_bpp=32; _depth=32; _red_mask=16711680; _green_mask=65280; _blue_mask=255; _alpha_mask=-16777216; _endianness=4321;
		}else if(strcmp(__format->format_label,"BGRA8888") == 0) { /*[Low Arrary Address] BGRABGRA...[High Array Address]*/
			gstcs_debug("BGRA8888");
			_bpp=32; _depth=32; _red_mask=65280; _green_mask=16711680; _blue_mask=-16777216; _alpha_mask=255; _endianness=4321;
		}else if(strcmp(__format->format_label,"RGBA8888") == 0) { /*[Low Arrary Address] RGBARGBA...[High Array Address]*/
			gstcs_debug("RGBA8888");
			_bpp=32; _depth=32; _red_mask=-16777216; _green_mask=16711680; _blue_mask=65280; _alpha_mask=255; _endianness=4321;
		}else if(strcmp(__format->format_label,"ABGR8888") == 0) { /*[Low Arrary Address] ABGRABGR...[High Array Address]*/
			gstcs_debug("ABGR8888");
			_bpp=32; _depth=32; _red_mask=255; _green_mask=65280; _blue_mask=16711680; _alpha_mask=-16777216; _endianness=4321;
		}else {
			gstcs_error("***Wrong format cs type***\n");
		}

		videoFormat = gst_video_format_from_masks(_depth, _bpp, _endianness, _red_mask, _green_mask, _blue_mask, _alpha_mask);
		gstcs_debug("Chosen video format: %s", gst_video_format_to_string(videoFormat));

		__format->caps = gst_caps_new_simple ("video/x-raw",
			"format", G_TYPE_STRING, gst_video_format_to_string(videoFormat),
			"width", G_TYPE_INT, __format->width,
			"height", G_TYPE_INT, __format->height,
			"framerate", GST_TYPE_FRACTION, 1, 1, NULL);
	}

	if(__format->caps) {
		gstcs_debug("###__format->caps is not NULL###, %p", __format->caps);
		_mm_check_caps_format(__format->caps);
	}else {
		gstcs_error("__format->caps is NULL");
	}
}

static void
_mm_set_image_colorspace( image_format_s* __format)
{
	gstcs_debug("format_label: %s\n", __format->format_label);

	__format->colorspace = (char*)malloc(sizeof(char) * IMAGE_FORMAT_LABEL_BUFFER_SIZE);
	if (__format->colorspace == NULL) {
		gstcs_error("memory allocation failed");
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
		gstcs_error("Check your colorspace format label");
		GSTCS_FREE(__format->colorspace);
	}
}

static void
_mm_round_up_input_image_widh_height(image_format_s* pFormat)
{
	if(strcmp(pFormat->colorspace,"YUV") == 0) {
		pFormat->stride = MM_UTIL_ROUND_UP_8(pFormat->width);
		pFormat->elevation = MM_UTIL_ROUND_UP_2(pFormat->height);
	} else if(strcmp(pFormat->colorspace, "RGB") == 0) {
		pFormat->stride = MM_UTIL_ROUND_UP_4(pFormat->width);
		pFormat->elevation = MM_UTIL_ROUND_UP_2(pFormat->height);
	} else if (strcmp(pFormat->colorspace,"RGBA") == 0){
		pFormat->stride = pFormat->width;
		pFormat->elevation = MM_UTIL_ROUND_UP_2(pFormat->height);
	}
	gstcs_debug("input_format stride: %d, elevation: %d", pFormat->stride, pFormat->elevation);
}

static image_format_s*
_mm_set_input_image_format_s_struct(imgp_info_s* pImgp_info) /* char* __format_label, int __width, int __height) */
{
	image_format_s* __format = NULL;

	__format=(image_format_s*)malloc(sizeof(image_format_s));
	if (__format == NULL) {
		gstcs_error("memory allocation failed");
		return NULL;
	}
	memset(__format, 0, sizeof(image_format_s));

	__format->format_label = (char *)malloc(sizeof(char) * IMAGE_FORMAT_LABEL_BUFFER_SIZE);
	if (__format->format_label == NULL) {
		gstcs_error("memory allocation failed");
		GSTCS_FREE(__format);
		return NULL;
	}
	memset(__format->format_label, 0, IMAGE_FORMAT_LABEL_BUFFER_SIZE);
	strncpy(__format->format_label, pImgp_info->input_format_label, strlen(pImgp_info->input_format_label));
	gstcs_debug("input_format_label: %s\n", pImgp_info->input_format_label);
	_mm_set_image_colorspace(__format);

	__format->width=pImgp_info->src_width;
	__format->height=pImgp_info->src_height;
	_mm_round_up_input_image_widh_height(__format);

	__format->blocksize = mm_setup_image_size(pImgp_info->input_format_label, pImgp_info->src_width, pImgp_info->src_height);
	gstcs_debug("blocksize: %d\n", __format->blocksize);
	_mm_set_image_input_format_s_capabilities(__format);

	return __format;
}

static void
_mm_round_up_output_image_widh_height(image_format_s* pFormat, const image_format_s *input_format)
{
	if(strcmp(pFormat->colorspace,"YUV") == 0) {
		pFormat->stride = MM_UTIL_ROUND_UP_8(pFormat->width);
		pFormat->elevation = MM_UTIL_ROUND_UP_2(pFormat->height);
	} else if(strcmp(pFormat->colorspace, "RGB") == 0) {
		pFormat->stride = MM_UTIL_ROUND_UP_4(pFormat->width);
		pFormat->elevation = MM_UTIL_ROUND_UP_2(pFormat->height);
		pFormat->width = pFormat->stride;
		if (input_format->height != input_format->elevation)
			pFormat->height = pFormat->elevation;
	} else if (strcmp(pFormat->colorspace,"RGBA") == 0){
		pFormat->stride = pFormat->width;
		pFormat->elevation = MM_UTIL_ROUND_UP_2(pFormat->height);
		if (input_format->height != input_format->elevation)
			pFormat->height = pFormat->elevation;
	}
	gstcs_debug("output_format stride: %d, elevation: %d", pFormat->stride, pFormat->elevation);
}

static image_format_s*
_mm_set_output_image_format_s_struct(imgp_info_s* pImgp_info, const image_format_s *input_format)
{
	image_format_s* __format = NULL;

	__format=(image_format_s*)malloc(sizeof(image_format_s));
	if (__format == NULL) {
		gstcs_error("memory allocation failed");
		return NULL;
	}
	memset(__format, 0, sizeof(image_format_s));

	__format->format_label = (char *)malloc(sizeof(char) * IMAGE_FORMAT_LABEL_BUFFER_SIZE);
	if (__format->format_label == NULL) {
		gstcs_error("memory allocation failed");
		GSTCS_FREE(__format);
		return NULL;
	}
	memset(__format->format_label, 0, IMAGE_FORMAT_LABEL_BUFFER_SIZE);
	strncpy(__format->format_label, pImgp_info->output_format_label, strlen(pImgp_info->output_format_label));
	_mm_set_image_colorspace(__format);

	__format->width=pImgp_info->dst_width;
	__format->height=pImgp_info->dst_height;
	_mm_round_up_output_image_widh_height(__format, input_format);

	pImgp_info->output_stride = __format->stride;
	pImgp_info->output_elevation = __format->elevation;

	__format->blocksize = mm_setup_image_size(pImgp_info->output_format_label, pImgp_info->dst_width, pImgp_info->dst_height);
	gstcs_debug("output_format_label: %s", pImgp_info->output_format_label);
	_mm_set_image_output_format_s_capabilities(__format);
	return __format;
}

static int
_mm_push_buffer_into_pipeline(imgp_info_s* pImgp_info, unsigned char *src, gstreamer_s * pGstreamer_s)
{
	int ret = GSTCS_ERROR_NONE;

	if(pGstreamer_s->pipeline == NULL) {
		gstcs_error("pipeline is NULL\n");
		return GSTCS_ERROR_INVALID_PARAMETER;
	}

	gsize data_size = mm_setup_image_size(pImgp_info->input_format_label, pImgp_info->src_width, pImgp_info->src_height);
	GstBuffer* gst_buf = gst_buffer_new_wrapped_full(GST_MEMORY_FLAG_READONLY, src, data_size, 0, data_size, NULL, NULL);

	if(gst_buf==NULL) {
		gstcs_error("buffer is NULL\n");
		return GSTCS_ERROR_INVALID_PARAMETER;
	}

	gst_app_src_push_buffer (GST_APP_SRC (pGstreamer_s->appsrc), gst_buf); /* push buffer to pipeline */
	return ret;
}

static int
_mm_push_buffer_into_pipeline_new(image_format_s *input_format, image_format_s *output_format, unsigned char *src, gstreamer_s * pGstreamer_s)
{
	int ret = GSTCS_ERROR_NONE;
	GstBuffer *gst_buf = NULL;
	unsigned int src_size = 0;
	unsigned char *data = NULL;
	unsigned int stride = input_format->stride;
	unsigned int elevation = input_format->elevation;

	if(pGstreamer_s->pipeline == NULL) {
		gstcs_error("pipeline is NULL\n");
		return GSTCS_ERROR_INVALID_PARAMETER;
	}

	gstcs_debug("stride: %d, elevation: %d", stride, elevation);
	src_size = mm_setup_image_size(input_format->format_label, stride, elevation);
	gstcs_debug("buffer size (src): %d", src_size);

	int byte_per_pixcel = _mm_get_byte_per_pixcel(input_format->format_label);
	unsigned int src_row = input_format->width * byte_per_pixcel;
	unsigned int stride_row = stride * byte_per_pixcel;
	unsigned int i = 0, y = 0;
	gstcs_debug("padding will be inserted to buffer");
	data =(unsigned char *) g_malloc (src_size);
	if(data==NULL) {
		gstcs_error("app_buffer is NULL\n");
		return GSTCS_ERROR_INVALID_PARAMETER;
	}
	for (y = 0; y < (unsigned int)(input_format->height); y++) {
		guint8 *pLine = (guint8 *) &(src[src_row * y]);
		for(i = 0; i < src_row; i++) {
			data[y * stride_row + i] = pLine[i];
		}
		for(i = src_row; i < stride_row; i++) {
			data[y * stride_row + i] = 0x00;
		}
	}
	for (y = (unsigned int)(input_format->height); y < (unsigned int)(input_format->elevation); y++) {
		for(i = 0; i < stride_row; i++) {
			data[y * stride_row + i] = 0x00;
		}
	}
	gst_buf = gst_buffer_new_wrapped_full(GST_MEMORY_FLAG_READONLY, data, src_size, 0, src_size, data, _mm_destroy_notify);

	if(gst_buf==NULL) {
		gstcs_error("buffer is NULL\n");
		return GSTCS_ERROR_INVALID_PARAMETER;
	}

	gst_app_src_push_buffer (GST_APP_SRC (pGstreamer_s->appsrc), gst_buf); /* push buffer to pipeline */
	return ret;
}

static int
_mm_imgp_gstcs_processing( gstreamer_s* pGstreamer_s, unsigned char *src, unsigned char *dst, image_format_s* input_format, image_format_s* output_format, imgp_info_s* pImgp_info)
{
	GstBus *bus = NULL;
	GstStateChangeReturn ret_state;
	int ret = GSTCS_ERROR_NONE;

	if(src== NULL || dst == NULL) {
		gstcs_error("src || dst is NULL");
		return GSTCS_ERROR_INVALID_PARAMETER;
	}

	/*create pipeline*/
	ret = _mm_create_pipeline(pGstreamer_s);
	if(ret != GSTCS_ERROR_NONE) {
		gstcs_error("ERROR - mm_create_pipeline ");
	}

	pGstreamer_s->context = g_main_context_new();
	if (pGstreamer_s->context == NULL) {
		gstcs_error("ERROR - g_main_context_new ");
		gst_object_unref (pGstreamer_s->pipeline);
		g_free (pGstreamer_s);
		return GSTCS_ERROR_INVALID_OPERATION;
	}
	pGstreamer_s->loop = g_main_loop_new (pGstreamer_s->context, FALSE);
	if (pGstreamer_s->loop == NULL) {
		gstcs_error("ERROR - g_main_loop_new ");
		gst_object_unref (pGstreamer_s->pipeline);
		g_main_context_unref(pGstreamer_s->context);
		g_free (pGstreamer_s);
		return GSTCS_ERROR_INVALID_OPERATION;
	}

	g_main_context_push_thread_default(pGstreamer_s->context);

	/* Make appsink emit the "new-preroll" and "new-sample" signals. This option is by default disabled because signal emission is expensive and unneeded when the application prefers to operate in pull mode. */
	gst_app_sink_set_emit_signals ((GstAppSink*)pGstreamer_s->appsink, TRUE);

	bus = gst_pipeline_get_bus (GST_PIPELINE (pGstreamer_s->pipeline));
	gst_bus_add_watch (bus, (GstBusFunc) _mm_on_src_message, pGstreamer_s);
	gst_object_unref(bus);

	if (((input_format->width != input_format->stride) || (input_format->height != input_format->elevation)) &&
		((strcmp(input_format->colorspace, "RGB") == 0) || (strcmp(input_format->colorspace, "RGBA") == 0))) {
		gstcs_debug("Start _mm_push_buffer_into_pipeline_new");
		ret = _mm_push_buffer_into_pipeline_new(input_format, output_format, src, pGstreamer_s);
	} else {
		gstcs_debug("Start mm_push_buffer_into_pipeline");
		ret = _mm_push_buffer_into_pipeline(pImgp_info, src, pGstreamer_s);
	}
	if(ret != GSTCS_ERROR_NONE) {
		gstcs_error("ERROR - mm_push_buffer_into_pipeline ");
		gst_object_unref (pGstreamer_s->pipeline);
		g_main_context_unref(pGstreamer_s->context);
		g_main_loop_unref(pGstreamer_s->loop);
		g_free (pGstreamer_s);
		return ret;
	}
	gstcs_debug("End mm_push_buffer_into_pipeline");

	/*link pipeline*/
	gstcs_debug("Start mm_link_pipeline");
	_mm_link_pipeline( pGstreamer_s, input_format, output_format, pImgp_info->angle);
	gstcs_debug("End mm_link_pipeline");

	/* Conecting to the new-sample signal emited by the appsink*/
	gstcs_debug("Start G_CALLBACK (_mm_sink_sample)");
	g_signal_connect (pGstreamer_s->appsink, "new-sample", G_CALLBACK (_mm_sink_sample), pGstreamer_s);
	gstcs_debug("End G_CALLBACK (_mm_sink_sample)");

	/* GST_STATE_PLAYING*/
	gstcs_debug("Start GST_STATE_PLAYING");
	ret_state = gst_element_set_state (pGstreamer_s->pipeline, GST_STATE_PLAYING);
	gstcs_debug("End GST_STATE_PLAYING ret_state: %d", ret_state);

	/*g_main_loop_run*/
	gstcs_debug("g_main_loop_run");
	g_main_loop_run (pGstreamer_s->loop);

	gstcs_debug("Sucess GST_STATE_CHANGE");

	/*GST_STATE_NULL*/
	gst_element_set_state (pGstreamer_s->pipeline, GST_STATE_NULL);
	gstcs_debug("End GST_STATE_NULL");

	gstcs_debug("###pGstreamer_s->output_buffer### : %p", pGstreamer_s->output_buffer);

	ret_state = gst_element_get_state (pGstreamer_s->pipeline, NULL, NULL, 1*GST_SECOND);

	if(ret_state == GST_STATE_CHANGE_SUCCESS) {
		gstcs_debug("GST_STATE_NULL ret_state = %d (GST_STATE_CHANGE_SUCCESS)\n", ret_state);
	}else if( ret_state == GST_STATE_CHANGE_ASYNC) {
		gstcs_debug("GST_STATE_NULL ret_state = %d (GST_STATE_CHANGE_ASYNC)\n", ret_state);
	}

	gstcs_debug("Success gst_element_get_state\n");

	if (ret_state == GST_STATE_CHANGE_FAILURE) {
		gstcs_error("GST_STATE_CHANGE_FAILURE");
	} else {
		if(pGstreamer_s->output_buffer != NULL) {
			GstMapInfo mapinfo = GST_MAP_INFO_INIT;
			gst_buffer_map(pGstreamer_s->output_buffer, &mapinfo, GST_MAP_READ);
			int buffer_size = mapinfo.size;
			int calc_buffer_size = 0;
			if (((pImgp_info->dst_width != output_format->width) || (pImgp_info->dst_height != output_format->height)) &&
				((strcmp(input_format->colorspace, "RGB") == 0) || (strcmp(input_format->colorspace, "RGBA") == 0))) {
				gstcs_debug("calculate image size with stride & elevation");
				calc_buffer_size = mm_setup_image_size(pImgp_info->output_format_label, output_format->width, output_format->height);
			} else {
				calc_buffer_size = mm_setup_image_size(pImgp_info->output_format_label, pImgp_info->dst_width, pImgp_info->dst_height);
			}
			gstcs_debug("buffer size: %d, calc: %d\n", buffer_size, calc_buffer_size);
			if( buffer_size != calc_buffer_size) {
				gstcs_debug("Buffer size is different \n");
				gstcs_debug("unref output buffer");
				gst_buffer_unref (pGstreamer_s->output_buffer);
				gst_object_unref (pGstreamer_s->pipeline);
				pGstreamer_s->output_buffer = NULL;
				g_main_context_unref(pGstreamer_s->context);
				g_main_loop_unref(pGstreamer_s->loop);
				g_free (pGstreamer_s);
				return GSTCS_ERROR_INVALID_OPERATION;
			}
			gstcs_debug("pGstreamer_s->output_buffer: 0x%2x\n", pGstreamer_s->output_buffer);
			memcpy(dst, mapinfo.data, buffer_size);
			pImgp_info->buffer_size = buffer_size;
			gst_buffer_unmap(pGstreamer_s->output_buffer, &mapinfo);
		}else {
			gstcs_debug("pGstreamer_s->output_buffer is NULL");
		}
	}
	gstcs_debug("unref output buffer");
	gst_buffer_unref (pGstreamer_s->output_buffer);
	gst_object_unref (pGstreamer_s->pipeline);
	pGstreamer_s->output_buffer = NULL;
	g_main_context_unref(pGstreamer_s->context);
	g_main_loop_unref(pGstreamer_s->loop);
	g_free (pGstreamer_s);

	gstcs_debug("End gstreamer processing");
	gstcs_debug("dst: %p", dst);
	return ret;
}


static int
mm_setup_image_size(const char* _format_label, int width, int height)
{
	int size=0;

	if(strcmp(_format_label, "I420") == 0) {
		size = (MM_UTIL_ROUND_UP_4(width) * MM_UTIL_ROUND_UP_2(height) + MM_UTIL_ROUND_UP_8(width) * MM_UTIL_ROUND_UP_2(height) /2); /*width * height *1.5; */
	}else if(strcmp(_format_label, "Y42B") == 0) {
		size = (MM_UTIL_ROUND_UP_4(width) * height + MM_UTIL_ROUND_UP_8(width) * height); /*width * height *2; */
	}else if(strcmp(_format_label, "YUV422") == 0) {
		size = (MM_UTIL_ROUND_UP_4(width) * height + MM_UTIL_ROUND_UP_8(width) * height); /*width * height *2; */
	}else if(strcmp(_format_label, "Y444") == 0) {
		size = (MM_UTIL_ROUND_UP_4(width) * height * 3); /* width * height *3; */
	}else if(strcmp(_format_label, "YV12") == 0) {
		size = (MM_UTIL_ROUND_UP_4(width) * MM_UTIL_ROUND_UP_2(height)+ MM_UTIL_ROUND_UP_8(width) * MM_UTIL_ROUND_UP_2(height) / 2); /* width * height *1; */
	}else if(strcmp(_format_label, "NV12") == 0) {
		size = (MM_UTIL_ROUND_UP_4(width) * MM_UTIL_ROUND_UP_2(height) * 1.5); /* width * height *1.5; */
	}else if(strcmp(_format_label, "RGB565") == 0) {
		size = (MM_UTIL_ROUND_UP_4(width) * 2 * height); /* width * height *2; */
	}else if(strcmp(_format_label, "RGB888") == 0) {
		size = (MM_UTIL_ROUND_UP_4(width) * 3 * height); /* width * height *3; */
	}else if(strcmp(_format_label, "BGR888") == 0) {
		size = (MM_UTIL_ROUND_UP_4(width) * 3 * height); /* width * height *3; */
	}else if(strcmp(_format_label, "UYVY") == 0) {
		size = (MM_UTIL_ROUND_UP_2(width) * 2 * height); /* width * height *2; */
	}else if(strcmp(_format_label, "YUYV") == 0) {
		size = (MM_UTIL_ROUND_UP_2(width) * 2 * height); /* width * height *2; */
	}else if(strcmp(_format_label, "ARGB8888") == 0) {
		size = width * height *4;
	}else if(strcmp(_format_label, "BGRA8888") == 0) {
		size = width * height *4;
	}else if(strcmp(_format_label, "RGBA8888") == 0) {
		size = width * height *4;
	}else if(strcmp(_format_label, "ABGR8888") == 0) {
		size = width * height *4;
	}else if(strcmp(_format_label, "BGRX") == 0) {
		size = width * height *4;
	}

	gstcs_debug("file_size: %d\n", size);

	return size;
}

static int
_mm_imgp_gstcs(imgp_info_s* pImgp_info, unsigned char *src, unsigned char *dst)
{
	image_format_s* input_format=NULL, *output_format=NULL;
	gstreamer_s* pGstreamer_s;
	int ret = GSTCS_ERROR_NONE;
	static const int max_argc = 50;
	gint* argc = NULL;
	gchar** argv = NULL;
	int i = 0;

	if(pImgp_info == NULL) {
		gstcs_error("imgp_info_s is NULL");
		return GSTCS_ERROR_INVALID_PARAMETER;
	}

	if(src== NULL || dst == NULL) {
		gstcs_error("src || dst is NULL");
		return GSTCS_ERROR_INVALID_PARAMETER;
	}
	gstcs_debug("[src %p] [dst %p]", src, dst);

	argc = malloc(sizeof(int));
	argv = malloc(sizeof(gchar*) * max_argc);

	if (!argc || !argv) {
		gstcs_error("argc ||argv is NULL");
		GSTCS_FREE(input_format);
		GSTCS_FREE(output_format);
		if (argc != NULL) {
			free(argc);
		}
		if (argv != NULL) {
			free(argv);
		}
		return GSTCS_ERROR_INVALID_PARAMETER;
	}
	memset(argv, 0, sizeof(gchar*) * max_argc);
	gstcs_debug("memset argv");

	/* add initial */
	*argc = 1;
	argv[0] = g_strdup( "mmutil_gstcs" );
	/* check disable registry scan */
	argv[*argc] = g_strdup("--gst-disable-registry-update");
	(*argc)++;
	gstcs_debug("--gst-disable-registry-update");

	gst_init(argc, &argv);

	pGstreamer_s = g_new0 (gstreamer_s, 1);

	for (i = 0; i < (*argc); i++) {
		GSTCS_FREE(argv[i]);
	}
	GSTCS_FREE(argv);
	GSTCS_FREE(argc);

	gstcs_debug("[input] format label : %s width: %d height: %d\t[output] format label: %s width: %d height: %d rotation vaule: %d",
	pImgp_info->input_format_label, pImgp_info->src_width, pImgp_info->src_height, pImgp_info->output_format_label, pImgp_info->dst_width, pImgp_info->dst_height, pImgp_info->angle);

	input_format= _mm_set_input_image_format_s_struct(pImgp_info);
	if (input_format == NULL) {
		gstcs_error("memory allocation failed");
		return GSTCS_ERROR_OUT_OF_MEMORY;
	}
	output_format= _mm_set_output_image_format_s_struct(pImgp_info, input_format);
	if (output_format == NULL) {
		gstcs_error("memory allocation failed");
		GSTCS_FREE(input_format->format_label);
		GSTCS_FREE(input_format->colorspace);
		GSTCS_FREE(input_format);
		return GSTCS_ERROR_OUT_OF_MEMORY;
	}

	/* _format_label : I420, RGB888 etc*/
	gstcs_debug("Start _mm_imgp_gstcs_processing ");
	ret =_mm_imgp_gstcs_processing(pGstreamer_s, src, dst, input_format, output_format, pImgp_info); /* input: buffer pointer for input image , input image format, input image width, input image height, output: buffer porinter for output image */

	if(ret == GSTCS_ERROR_NONE) {
		gstcs_debug("End _mm_imgp_gstcs_processing [dst: %p]", dst);
	}else if (ret != GSTCS_ERROR_NONE) {
		gstcs_error("ERROR - _mm_imgp_gstcs_processing");
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
		gstcs_error("input vaule is error");
	}
	return _mm_imgp_gstcs(pImgp_info, src, dst);
}
