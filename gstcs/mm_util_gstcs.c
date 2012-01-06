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
#include <mm_error.h>
#define MM_UTIL_ROUND_UP_2(num)  (((num)+1)&~1)
#define MM_UTIL_ROUND_UP_4(num)  (((num)+3)&~3)
#define MM_UTIL_ROUND_UP_8(num)  (((num)+7)&~7)
#define MM_UTIL_ROUND_UP_16(num)  (((num)+15)&~15)
/*########################################################################################*/
#define setup_image_size_I420(width, height) { \
	int size=0; \
	size = (MM_UTIL_ROUND_UP_4 (width) * MM_UTIL_ROUND_UP_2 (height) + MM_UTIL_ROUND_UP_8 (width) * MM_UTIL_ROUND_UP_2 (height) /2); \
	return size; \
}

#define setup_image_size_Y42B(width, height)  { \
	int size=0; \
	size = (MM_UTIL_ROUND_UP_4 (width) * height + MM_UTIL_ROUND_UP_8 (width)  * height); \
	return size; \
}	

#define setup_image_size_Y444(width, height) { \
	int size=0; \
	size = (MM_UTIL_ROUND_UP_4 (width) * height  * 3); \
	return size; \
}

#define setup_image_size_UYVY(width, height) { \
	int size=0; \
	size = (MM_UTIL_ROUND_UP_2 (width) * 2 * height); \
	return size; \
}

#define setup_image_size_YUYV(width, height)  { \
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

#define setup_image_size_RGB565(width, height)  { \
	int size=0; \
	size = (MM_UTIL_ROUND_UP_4 (width * 2) *height); \
	return size; \
}

#define setup_image_size_RGB888(width, height)  { \
	int size=0; \
	size = (MM_UTIL_ROUND_UP_4 (width*3) * height); \
	return size; \
}

#define setup_image_size_BGR888(width, height)  { \
	int size=0; \
	size = (MM_UTIL_ROUND_UP_4 (width*3) * height); \
	return size; \
}
/*########################################################################################*/

static void 
_mm_sink_buffer (GstElement * appsink, gpointer  user_data)
{
	GstBuffer *_buf=NULL;
	gstreamer * gstreamer_h = (gstreamer*) user_data;	
	_buf = gst_app_sink_pull_buffer((GstAppSink*)appsink);		

	gstreamer_h->output_buffer = _buf; 
	
	if(gstreamer_h->output_buffer != NULL)	 
	{
		mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] Create Output Buffer: GST_BUFFER_DATA: %p\t GST_BUFFER_SIZE: %d\n", __func__, __LINE__, GST_BUFFER_DATA(gstreamer_h->output_buffer),  GST_BUFFER_SIZE (gstreamer_h->output_buffer));
	}
	else 
	{
		mmf_debug(MMF_DEBUG_ERROR,"[%s][%05d] ERROR -Input Prepare Buffer!  Check createoutput buffer function", __func__, __LINE__);
	}
	 gst_buffer_unref (_buf);   //we don't need the appsink buffer anymore
	 gst_buffer_ref  (gstreamer_h->output_buffer); //when you want to avoid flushing 
}

static void 
_mm_sink_preroll (GstElement * appsink, gpointer  user_data)
{
	GstBuffer *_buf=NULL;
	gstreamer * gstreamer_h = (gstreamer*) user_data;	
	_buf = gst_app_sink_pull_preroll((GstAppSink*)appsink);		

	gstreamer_h->output_buffer = _buf; 
	//gstreamer_h->output_image_format->caps =  GST_BUFFER_CAPS(_buf);
	if(gstreamer_h->output_buffer != NULL)	 
	{
		mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] Create Output Buffer: GST_BUFFER_DATA: %p\t GST_BUFFER_SIZE: %d\n", __func__, __LINE__,
			GST_BUFFER_DATA(gstreamer_h->output_buffer),  GST_BUFFER_SIZE (gstreamer_h->output_buffer));
	}
	else 
	{
		mmf_debug(MMF_DEBUG_ERROR,"[%s][%05d] ERROR -Input Prepare Buffer!  Check createoutput buffer function", __func__, __LINE__);
	}
	
	 gst_buffer_unref (_buf);   // we don't need the appsink buffer anymore
	 gst_buffer_ref  (gstreamer_h->output_buffer); //when you want to avoid flushing 
}

static gboolean 
_mm_on_sink_message  (GstBus * bus, GstMessage * message, gstreamer * gstreamer_h)
{

	switch (GST_MESSAGE_TYPE (message)) 
	{
		case GST_MESSAGE_EOS:
			mmf_debug(MMF_DEBUG_LOG,"Finished playback\n"); 			//g_main_loop_quit (gstreamer_h->loop);
			break;
		case GST_MESSAGE_ERROR:
			mmf_debug(MMF_DEBUG_ERROR,"Received error\n"); 			//g_main_loop_quit (gstreamer_h->loop);
		  	break;
		case GST_MESSAGE_STATE_CHANGED:
			mmf_debug(MMF_DEBUG_LOG, " [%s] %s(%d) \n", 	GST_MESSAGE_SRC_NAME(message), GST_MESSAGE_TYPE_NAME(message), GST_MESSAGE_TYPE(message));
			break;	
		case GST_MESSAGE_STREAM_STATUS:
			mmf_debug(MMF_DEBUG_LOG, " [%s] %s(%d) \n",  GST_MESSAGE_SRC_NAME(message),  GST_MESSAGE_TYPE_NAME(message), GST_MESSAGE_TYPE(message));
			break;			
		default: 
			mmf_debug(MMF_DEBUG_LOG, " [%s] %s(%d) \n",  GST_MESSAGE_SRC_NAME(message), GST_MESSAGE_TYPE_NAME(message), GST_MESSAGE_TYPE(message));
			break;
	}
	return TRUE;
}


static int
_mm_create_pipeline( gstreamer* gstreamer_h)
{
	int ret = MM_ERROR_NONE;
	gstreamer_h->pipeline= gst_pipeline_new ("ffmpegcolorsapce");
	gstreamer_h->appsrc= gst_element_factory_make("appsrc","appsrc");
	gstreamer_h->colorspace=gst_element_factory_make("ffmpegcolorspace","colorconverter");

	gstreamer_h->videoscale=gst_element_factory_make("videoscale", "scale");
	gstreamer_h->videoflip=gst_element_factory_make( "videoflip", "flip" );

	gstreamer_h->appsink=gst_element_factory_make("appsink","appsink");

	if (!gstreamer_h->pipeline || !gstreamer_h->appsrc||!gstreamer_h->colorspace || !gstreamer_h->appsink) 
	{
		mmf_debug(MMF_DEBUG_ERROR,"[%s][%05d] One element could not be created. Exiting.\n", __func__, __LINE__);
		ret = MM_ERROR_IMAGE_INVALID_VALUE;
	}
	return ret;
}
	
static gboolean
_mm_check_resize_format(int src_width, int src_height, int dst_width, int dst_height)
{
	gboolean _bool=FALSE;

	if( (src_width != dst_width) || (src_height != dst_height) )
	{
		_bool = TRUE;
	}

	return _bool;		
}

static gboolean
_mm_check_rotate_format(int GstVideoFlipMethod)
{
	gboolean _bool=FALSE;

	if((GstVideoFlipMethod >= 1) && ( GstVideoFlipMethod<= 7))
	{
		_bool = TRUE;
	}

	return _bool;		
}

static gboolean
_mm_check_resize_format_label(char* __format_label)
{
	 gboolean _bool = FALSE;

	if(strcmp(__format_label, "AYUV") == 0
		|| strcmp(__format_label, "UYVY") == 0 ||strcmp(__format_label, "Y800") == 0 || strcmp(__format_label, "I420") == 0  || strcmp(__format_label, "YV12") == 0
		|| strcmp(__format_label, "RGB888") == 0  || strcmp(__format_label, "RGB565") == 0 || strcmp(__format_label, "BGR888") == 0  || strcmp(__format_label, "RGBA8888") == 0 
		|| strcmp(__format_label, "ARGB8888") == 0 ||strcmp(__format_label, "BGRA8888") == 0 ||strcmp(__format_label, "ABGR8888") == 0 ||strcmp(__format_label, "RGBX") == 0 
		||strcmp(__format_label, "XRGB") == 0 ||strcmp(__format_label, "BGRX") == 0 ||strcmp(__format_label, "XBGR") == 0 ||strcmp(__format_label, "Y444") == 0
		||strcmp(__format_label, "Y42B") == 0 ||strcmp(__format_label, "YUY2") == 0 ||strcmp(__format_label, "YUYV") == 0 ||strcmp(__format_label, "UYVY") == 0
    		||strcmp(__format_label, "Y41B") == 0 ||strcmp(__format_label, "Y16") == 0 ||strcmp(__format_label, "Y800") == 0 ||strcmp(__format_label, "Y8") == 0
		||strcmp(__format_label, "GREY") == 0 ||strcmp(__format_label, "AY64") == 0 || strcmp(__format_label, "YUV422") == 0)
	{
		_bool=TRUE;
	}		

	return _bool;
}

static gboolean
_mm_check_rotate_format_label(const char* __format_label)
{
	 gboolean _bool = FALSE;

	if(strcmp(__format_label, "I420") == 0 ||strcmp(__format_label, "YV12") == 0 || strcmp(__format_label, "IYUV") == 0)
	{
		_bool=TRUE;
	}		

	return _bool;
}

static void
_mm_link_pipeline_order_csc_rsz(gstreamer* gstreamer_h, image_format*  input_format, image_format* output_format)
{

	if(_mm_check_resize_format(input_format->width,input_format->height, output_format->width, output_format->height))
	{
		mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] check_for_resize", __func__, __LINE__);
		gst_bin_add_many(GST_BIN(gstreamer_h->pipeline), gstreamer_h->appsrc, gstreamer_h->videoscale, gstreamer_h->colorspace, gstreamer_h->appsink,  NULL);			
		mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] gst_bin_add_many", __func__, __LINE__);
		if(_mm_check_resize_format_label( input_format->format_label))
		{
			mmf_debug(MMF_DEBUG_LOG, "[%s][%05d]  input_format->format_label: %s", __func__, __LINE__,  input_format->format_label);
			if(!gst_element_link_many(gstreamer_h->appsrc, gstreamer_h->videoscale,  gstreamer_h->colorspace,  gstreamer_h->appsink, NULL)) 		mmf_debug(MMF_DEBUG_ERROR,"[%s][%05d] Fail to link b/w ffmpeg and appsink except rot\n", __func__, __LINE__);
		}
		else if(_mm_check_resize_format_label(output_format->format_label))
		{	
			mmf_debug(MMF_DEBUG_LOG, "[%s][%05d]  output_format->format_label: %s", __func__, __LINE__,  output_format->format_label);		
			if(!gst_element_link_many(gstreamer_h->appsrc,  gstreamer_h->colorspace, gstreamer_h->videoscale, gstreamer_h->appsink, NULL)) 		mmf_debug(MMF_DEBUG_ERROR,"[%s][%05d] Fail to link b/w ffmpeg and appsink except rot\n", __func__, __LINE__);
		}
	}	

	else
	{
		mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] check_for_convert", __func__, __LINE__);
		gst_bin_add_many(GST_BIN(gstreamer_h->pipeline), gstreamer_h->appsrc, gstreamer_h->colorspace, gstreamer_h->appsink,  NULL);	
		mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] gst_bin_add_many", __func__, __LINE__);
		if(!gst_element_link_many(gstreamer_h->appsrc,  gstreamer_h->colorspace, gstreamer_h->appsink, NULL)) 		mmf_debug(MMF_DEBUG_ERROR,"[%s][%05d] Fail to link b/w ffmpeg and appsink except rsz & rot\n", __func__, __LINE__);
	}

}

static void 
_mm_link_pipeline_order_csc_rsz_rot(gstreamer* gstreamer_h, image_format*  input_format, image_format* output_format)
{
	
	if(_mm_check_rotate_format_label(input_format->format_label)) 
	{
		gst_bin_add_many(GST_BIN(gstreamer_h->pipeline), gstreamer_h->appsrc,  gstreamer_h->videoscale, gstreamer_h->videoflip, gstreamer_h->colorspace, gstreamer_h->appsink,  NULL);			

		if(!gst_element_link_many(gstreamer_h->appsrc, gstreamer_h->videoscale,  gstreamer_h->videoflip,  gstreamer_h->colorspace,  gstreamer_h->appsink, NULL)) 		mmf_debug(MMF_DEBUG_ERROR,"[%s][%05d] Fail to link b/w appsrc and ffmpeg in rotate\n", __func__, __LINE__);
	}

	else if(_mm_check_rotate_format_label(output_format->format_label)) 
	{
		gst_bin_add_many(GST_BIN(gstreamer_h->pipeline), gstreamer_h->appsrc,  gstreamer_h->colorspace, gstreamer_h->videoscale,  gstreamer_h->videoflip,gstreamer_h->appsink,  NULL);			
		if(!gst_element_link_many(gstreamer_h->appsrc,  gstreamer_h->colorspace,  gstreamer_h->videoscale, gstreamer_h->videoflip, gstreamer_h->appsink, NULL)) 		mmf_debug(MMF_DEBUG_ERROR,"[%s][%05d]] Fail to link b/w ffmpeg and appsink in rotate\n", __func__, __LINE__);	
	}
}


static void
_mm_link_pipeline( gstreamer* gstreamer_h, image_format* input_format, image_format* output_format, int _valuegstreamer_hVideoFlipMethod)
{
	/* set property */
	gst_app_src_set_caps(GST_APP_SRC(gstreamer_h->appsrc), input_format->caps); //g_object_set(gstreamer_h->appsrc, "caps", input_format->caps, NULL);  //  you can use appsrc'cap property
	g_object_set(gstreamer_h->appsrc, "num-buffers", 1, NULL);
	g_object_set(gstreamer_h->appsrc, "is-live", TRUE, NULL);	// add because of gstreamer time issue
	g_object_set (gstreamer_h->appsrc, "format", GST_FORMAT_TIME, NULL);
	g_object_set(gstreamer_h->appsrc, "stream-type", 0 /*stream*/, NULL);

	g_object_set(gstreamer_h->videoflip, "method", _valuegstreamer_hVideoFlipMethod, NULL ); // GST_VIDEO_FLIP_METHOD_IDENTITY   (0): none- Identity (no rotation)   (1): clockwise  - Rotate clockwise 90 degrees   (2): rotate-180   - Rotate 180 degrees  (3): counterclockwise - Rotate counter-clockwise 90 degrees  (4): horizontal-flip  - Flip horizontally   (5): vertical-flip    - Flip vertically   (6): upper-left-diagonal - Flip across upper left/lower right diagonal  (7): upper-right-diagonal - Flip across upper right/lower left diagonal		

	gst_app_sink_set_caps(GST_APP_SINK(gstreamer_h->appsink), output_format->caps); 	//g_object_set(gstreamer_h->appsink, "caps", output_format->caps, NULL);		
	g_object_set(gstreamer_h->appsink,  "drop", TRUE, NULL);	
	g_object_set(gstreamer_h->appsink, "emit-signals", TRUE, "sync", FALSE, NULL);

	if(_mm_check_rotate_format(_valuegstreamer_hVideoFlipMethod))  // when you want to rotate image
	{
		/*  because IYUV, I420, YV12 format can use vidoeflip*/
		mmf_debug(MMF_DEBUG_LOG, "[%s][%05d]  set_link_pipeline_order_csc_rsz_rot", __func__, __LINE__);
		_mm_link_pipeline_order_csc_rsz_rot(gstreamer_h,  input_format, output_format);
	}
	
	else
	{			
		mmf_debug(MMF_DEBUG_LOG, "[%s][%05d]  set_link_pipeline_order_csc_rsz", __func__, __LINE__);
		_mm_link_pipeline_order_csc_rsz(gstreamer_h, input_format,  output_format);
	}
	/* Conecting to the new-buffer signal emited by the appsink*/ 
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] Start  G_CALLBACK (mm_sink_buffer)", __func__, __LINE__);
	g_signal_connect (gstreamer_h->appsink, "new-buffer",  G_CALLBACK (_mm_sink_buffer), gstreamer_h); 
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] End  G_CALLBACK (mm_sink_buffer)", __func__, __LINE__)
}



static void
_mm_set_image_format_capabilities(image_format* __format)//_format_label: I420 _colorsapace: YUV
{
	char _a='A', _b='A', _c='A', _d='A';
	int _bpp=0; int _depth=0; int _red_mask=0; int _green_mask=0; int  _blue_mask=0; int _alpha_mask=0; int _endianness=0;
	
	if(__format == NULL)
	{
		 mmf_debug(MMF_DEBUG_ERROR,"[%s][%05d] Image format is NULL\n", __func__, __LINE__);
	}
	__format->caps = NULL;
	//g_type_init();
	
	mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] colorspace: %s\n", __func__, __LINE__, __format->colorspace);
	
	if(strcmp(__format->colorspace,"YUV") == 0)
	{
		if(strcmp(__format->format_label,"I420") == 0) {_a='I'; _b='4', _c='2', _d='0'; }
		else if(strcmp(__format->format_label,"Y42B") == 0) {_a='Y'; _b='4', _c='2', _d='B'; }
		else if(strcmp(__format->format_label,"Y444") == 0) {_a='Y'; _b='4', _c='4', _d='4'; } 
		else if(strcmp(__format->format_label,"YV12") == 0) {_a='Y'; _b='V', _c='1', _d='2'; }  //GStreamer-CRITICAL **: gst_mini_object_ref: assertion `mini_object != NULL' failed
		else if(strcmp(__format->format_label,"NV12") == 0) {_a='N'; _b='V', _c='1', _d='2'; }				
		else if(strcmp(__format->format_label,"UYVY") == 0) {_a='U'; _b='Y', _c='V', _d='Y'; }
		else if(strcmp(__format->format_label,"YUYV") == 0) {_a='Y'; _b='U', _c='Y', _d='2'; }
		
		__format->caps =  gst_caps_new_simple ("video/x-raw-yuv",
			 "format", GST_TYPE_FOURCC, GST_MAKE_FOURCC (_a, _b, _c, _d), //'I', '4', '2', '0'),
 			 "framerate", GST_TYPE_FRACTION, 25, 1,
			 "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
			 "width", G_TYPE_INT, __format->width,//320,
			 "height", G_TYPE_INT, __format->height, //240,
			 "framerate", GST_TYPE_FRACTION, 1, 1,
			 NULL);

	}
	
	else if(strcmp(__format->colorspace,"RGB") ==0 || strcmp(__format->colorspace,"BGRX") ==0)
	{
		if(strcmp(__format->format_label,"RGB888") == 0) { _bpp=24; _depth=24;  _red_mask=16711680; _green_mask=65280; _blue_mask=255; _endianness=4321; }
		else if(strcmp(__format->format_label,"BGR888") == 0) { _bpp=24; _depth=24;  _red_mask=255; _green_mask=65280; _blue_mask=16711680; _endianness=4321; }
		else if(strcmp(__format->format_label,"RGB565") == 0) { _bpp=16; _depth=16;  _red_mask=63488; _green_mask=2016; _blue_mask=31; _endianness=1234; }
		else if( (strcmp(__format->format_label, "BGRX") == 0)) { _bpp=32; _depth=24;  _red_mask=65280; _green_mask=16711680; _blue_mask=-16777216;  _endianness=4321; }
		 __format->caps  =  gst_caps_new_simple ("video/x-raw-rgb",
		          "bpp", G_TYPE_INT, _bpp,
		          "depth", G_TYPE_INT, _depth,
		          "red_mask", G_TYPE_INT, _red_mask,
		          "green_mask", G_TYPE_INT, _green_mask,
		          "blue_mask", G_TYPE_INT, _blue_mask,
		          "width", G_TYPE_INT, __format->width, 
		          "height", G_TYPE_INT, __format->height,
		          "endianness", G_TYPE_INT, _endianness,		          
		          "framerate", GST_TYPE_FRACTION, 1, 1, NULL);
	}
	
	else if(strcmp(__format->colorspace,"RGBA") ==0)
	{
		/*[Low Arrary Address] ARGBARGB... [High Array Address]*/
		if(strcmp(__format->format_label,"ARGB8888") == 0) { _bpp=32; _depth=32;  _red_mask=16711680; _green_mask=65280; _blue_mask=255; _alpha_mask=-16777216; _endianness=4321; }

		/*[Low Arrary Address] BGRABGRA...[High Array Address]*/
		else if(strcmp(__format->format_label,"BGRA8888") == 0)  { _bpp=32; _depth=32;  _red_mask=65280; _green_mask=16711680; _blue_mask=-16777216; _alpha_mask=255; _endianness=4321; }

		/*[Low Arrary Address] RGBARGBA...[High Array Address]*/
		else if(strcmp(__format->format_label,"RGBA8888") == 0)  { _bpp=32; _depth=32;  _red_mask=-16777216; _green_mask=16711680; _blue_mask=65280; _alpha_mask=255; _endianness=4321; }

		/*[Low Arrary Address] ABGRABGR...[High Array Address]*/
		else if(strcmp(__format->format_label,"ABGR8888") == 0)  { _bpp=32; _depth=32;  _red_mask=255; _green_mask=65280; _blue_mask=16711680; _alpha_mask=-16777216; _endianness=4321; }
		else mmf_debug(MMF_DEBUG_ERROR,"[%s][%05d] ***Wrong format cs type***\n", __func__, __LINE__);

		__format->caps =  gst_caps_new_simple ("video/x-raw-rgb",
		          "bpp", G_TYPE_INT, _bpp,
		          "depth", G_TYPE_INT, _depth,
		          "red_mask", G_TYPE_INT, _red_mask,
		          "green_mask", G_TYPE_INT, _green_mask,
		          "blue_mask", G_TYPE_INT, _blue_mask,
		          "width", G_TYPE_INT, __format->width, 
		          "height", G_TYPE_INT, __format->height,
		          "alpha_mask", G_TYPE_INT,_alpha_mask,
		          "endianness", G_TYPE_INT, _endianness,		          
		          "framerate", GST_TYPE_FRACTION, 1, 1, NULL);
	}
	if(__format->caps) 
	{	
		mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] ###__format->caps is not  NULL###, %p", __func__, __LINE__, __format->caps);
	}
	else 
	{	
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] __format->caps is NULL", __func__, __LINE__);		
	}
}

static void
_mm_set_image_colorspace( image_format* __format)
{
	mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] format_label: %s\n", __func__, __LINE__, __format->format_label);
	if( (strcmp(__format->format_label, "I420") == 0) ||(strcmp(__format->format_label, "Y42B") == 0) || (strcmp(__format->format_label, "Y444") == 0) 
		|| (strcmp(__format->format_label, "YV12") == 0) ||(strcmp(__format->format_label, "NV12") == 0)  ||(strcmp(__format->format_label, "UYVY") == 0) ||(strcmp(__format->format_label, "YUYV") == 0)) 
	{
		strncpy(__format->colorspace, "YUV", sizeof("__format->colorspace"));
	}		
	else	if( (strcmp(__format->format_label, "RGB888") == 0) ||(strcmp(__format->format_label, "BGR888") == 0) ||(strcmp(__format->format_label, "RGB565") == 0))
	{
		strncpy(__format->colorspace, "RGB",  sizeof("__format->colorspace"));
	}
	else	if( (strcmp(__format->format_label, "ARGB8888") == 0)  || (strcmp(__format->format_label, "BGRA8888") == 0)
			||(strcmp(__format->format_label, "RGBA8888") == 0)	|| (strcmp(__format->format_label, "ABGR8888") == 0))		
	{
		strncpy(__format->colorspace, "RGBA",  sizeof("__format->colorspace"));
	}
	else	if( (strcmp(__format->format_label, "BGRX") == 0))
	{
		strncpy(__format->colorspace, "BGRX",  sizeof("__format->colorspace"));
	}
	else mmf_debug(MMF_DEBUG_ERROR,"[%s][%05d] Check your colorspace format label", __func__, __LINE__);
}

static image_format*
_mm_set_input_image_format_struct(imgp_info* _imgp_info) //char* __format_label, int __width, int __height)
{	
	 image_format* __format = NULL;

	__format=(image_format*)malloc(sizeof(image_format));
	strncpy(__format->format_label, _imgp_info->input_format_label, sizeof(__format->format_label));
	mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] input_format_label: %s\n", __func__, __LINE__, _imgp_info->input_format_label);
	_mm_set_image_colorspace(__format);	

	__format->width=_imgp_info->src_width; 
	__format->height=_imgp_info->src_height;

	__format->blocksize = mm_setup_image_size(_imgp_info->input_format_label, _imgp_info->src_width, _imgp_info->src_height);
	mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] input_format_label: %s\n", __func__, __LINE__, _imgp_info->input_format_label);
	_mm_set_image_format_capabilities(__format);

	return __format;
}

static void
_mm_round_up_output_image_widh_height(char* __colorsapce, imgp_info* _imgp_info) //char* __colorspace, int __width, int __height)
{	
	if(strcmp(__colorsapce,"YUV") ==0)
	{
		_imgp_info->src_width =GST_ROUND_UP_8(_imgp_info->src_width); 		// because the result of image size is multiples of 8
		_imgp_info->src_height=GST_ROUND_UP_8(_imgp_info->src_height);		
	}
	else if(strcmp(__colorsapce, "RGB") ==0)
	{		
		_imgp_info->src_width=GST_ROUND_UP_4(_imgp_info->src_width); 			
	}
}

static image_format*
_mm_set_output_image_format_struct(imgp_info* _imgp_info) 
{	
	 image_format* __format = NULL;

	__format=(image_format*)malloc(sizeof(image_format));
	strncpy(__format->format_label, _imgp_info->output_format_label, sizeof(__format->format_label));
	_mm_set_image_colorspace(__format);	
	_mm_round_up_output_image_widh_height(__format->colorspace, _imgp_info);
	__format->width=_imgp_info->dst_width; 
	__format->height=_imgp_info->dst_height;

	__format->blocksize = mm_setup_image_size(_imgp_info->output_format_label, _imgp_info->dst_width, _imgp_info->dst_height);
	mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] output_format_label: %s", __func__, __LINE__, _imgp_info->output_format_label);
	_mm_set_image_format_capabilities(__format);
	mmf_debug (MMF_DEBUG_LOG, "[%s][%05d] _imgp_info->dst: %p", __func__, __LINE__, _imgp_info->dst);
	return __format;
}

static gboolean
__mm_check_resize_format( char* _in_format_label, int _in_w, int _in_h, char* _out_format_label, int _out_w, int _out_h)
{
	gboolean _bool=TRUE;
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] input image format: %s input image width: %d input image height: %d output image format: %s output image width: %d output image height: %d\n",
	__func__, __LINE__,_in_format_label, _in_w,_in_h, _out_format_label, _out_w, _out_h);
	if(_mm_check_resize_format(_in_w, _in_h, _out_w, _out_h))
	{
		if( !( _mm_check_resize_format_label(_in_format_label) ||_mm_check_resize_format_label(_out_format_label) ) )
		{
			mmf_debug(MMF_DEBUG_ERROR,"[%s][%05d] ERROR -%s | %s  Resize  Only [AYUV] [YUY2] [YVYU] [UYVY] [Y800] [I420][YV12]  format can use vidoeflip ERROR- Resize!!!#####\n", __func__, __LINE__, _in_format_label, _out_format_label);			
			_bool = FALSE;
		}
	}
	return _bool;
}

static gboolean 
__mm_check_rotate_format(int angle, const char* input_format_label, const char* output_format_label)
{
	gboolean _bool=TRUE;
	mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] rotate value: %d  input_format_label: %s output_format_label: %s\n", 
	__func__, __LINE__, angle, input_format_label, output_format_label);	
	if(_mm_check_rotate_format(angle))
	{
		if(!(_mm_check_rotate_format_label(input_format_label) || _mm_check_rotate_format_label(output_format_label)))
		{
			mmf_debug(MMF_DEBUG_ERROR,"[%s][%05d]  ERROR - %s  | %s  Rotate  Only IYUV, I420, YV12 format can use vidoeflip ERROR -Rotate", __func__, __LINE__, input_format_label, output_format_label);			
			_bool = FALSE;
		}
	}
	return _bool;
}

static int 
_mm_push_buffer_into_pipeline(imgp_info* _imgp_info, gstreamer * gstreamer_h, GstCaps*_caps)
{	
	int ret = MM_ERROR_NONE;
	if(_caps==NULL)
	{
		mmf_debug(MMF_DEBUG_ERROR,"[%s][%05d] caps is NULL\n", __func__, __LINE__);
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}
	
	if(gstreamer_h->pipeline == NULL) 
	{
		mmf_debug(MMF_DEBUG_ERROR,"[%s][%05d] pipeline is NULL\n", __func__, __LINE__);		
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}

	GstBuffer* gst_buf = (GstBuffer *) gst_mini_object_new (GST_TYPE_BUFFER);

	if(gst_buf==NULL)
	{
		mmf_debug(MMF_DEBUG_ERROR,"[%s][%05d] buffer is NULL\n", __func__, __LINE__);
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}
	GST_BUFFER_DATA (gst_buf) = (guint8 *) _imgp_info->src;
	GST_BUFFER_SIZE (gst_buf) = mm_setup_image_size(_imgp_info->input_format_label, _imgp_info->src_width, _imgp_info->src_height);
	GST_BUFFER_FLAG_SET (gst_buf, GST_BUFFER_FLAG_READONLY);

	gst_buffer_set_caps (gst_buf, _caps);
	gst_app_src_push_buffer (GST_APP_SRC (gstreamer_h->appsrc), gst_buf); //push buffer to pipeline
	g_free(GST_BUFFER_MALLOCDATA(gst_buf)); gst_buf = NULL; //gst_buffer_finalize(gst_buf) { buffer->free_func (buffer->malloc_data); }
	mmf_debug (MMF_DEBUG_LOG, "[%s][%05d] #g_free#gst_buf: 0x%2x\n", __func__, __LINE__, gst_buf);
	return ret;
}


static int
_mm_imgp_gstcs_processing( gstreamer* gstreamer_h, image_format* input_format, image_format* output_format, imgp_info* _imgp_info) 
{					
	GstBus *bus = NULL;
	GstStateChangeReturn ret_state;
	int ret = MM_ERROR_NONE;
	/*create pipeline*/
	ret =  _mm_create_pipeline(gstreamer_h);
	if(ret != MM_ERROR_NONE)
	{
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] ERROR - mm_create_pipeline ", __func__, __LINE__);
	}
	

   	/*  Make appsink emit the "new-preroll" and "new-buffer" signals. This  option is by default disabled because  signal emission is expensive and unneeded when the application prefers  to operate in pull mode.   */
	 gst_app_sink_set_emit_signals ((GstAppSink*)gstreamer_h->appsink, TRUE);

	bus = gst_pipeline_get_bus (GST_PIPELINE (gstreamer_h->pipeline)); //GST_PIPELINE (pipeline));
	gst_bus_add_watch (bus, (GstBusFunc) _mm_on_sink_message , gstreamer_h); // thow to  appplicaton
	gst_object_unref(bus);

	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] Start mm_push_buffer_into_pipeline", __func__, __LINE__);
	ret = _mm_push_buffer_into_pipeline(_imgp_info, gstreamer_h, input_format->caps);
	if(ret != MM_ERROR_NONE)
	{
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] ERROR - mm_push_buffer_into_pipeline ", __func__, __LINE__);
	}
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] End mm_push_buffer_into_pipeline", __func__, __LINE__);
	
	/*link pipeline*/		
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] Start mm_link_pipeline", __func__, __LINE__);
	_mm_link_pipeline( gstreamer_h, input_format,  output_format, _imgp_info->angle);	
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] End mm_link_pipeline", __func__, __LINE__);

	/* Conecting to the new-buffer signal emited by the appsink*/ 
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] Start  G_CALLBACK (_mm_sink_preroll)", __func__, __LINE__);
	g_signal_connect (gstreamer_h->appsink, "new-preroll",  G_CALLBACK (_mm_sink_preroll), gstreamer_h); 
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] End  G_CALLBACK (mm_sink_preroll)", __func__, __LINE__);	

	/* GST_STATE_PLAYING*/
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] Start GST_STATE_PLAYING", __func__, __LINE__);
	ret_state = gst_element_set_state (gstreamer_h->pipeline, GST_STATE_PLAYING);
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] End GST_STATE_PLAYING ret_state: %d", __func__, __LINE__, ret_state);
	ret_state = gst_element_get_state (gstreamer_h->pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] Success Get GST_STATE_PLAYING ret_state: %d", __func__, __LINE__, ret_state);	
        #if 0
	/* Conecting to the new-buffer signal emited by the appsink*/ 
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] Start  G_CALLBACK (mm_sink_buffer)", __func__, __LINE__);
	g_signal_connect (gstreamer_h->appsink, "new-buffer",  G_CALLBACK (_mm_sink_buffer), gstreamer_h); 
	mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] End  G_CALLBACK (mm_sink_buffer)", __func__, __LINE__);
	#endif
	if(ret_state  == 1)   	{ mmf_debug (MMF_DEBUG_LOG, "[%s][%05d] GST_STATE_PLAYING ret = %d( GST_STATE_CHANGE_SUCCESS)", __func__, __LINE__, ret_state);         }
	else if( ret_state == 2)  { mmf_debug (MMF_DEBUG_LOG, "[%s][%05d] GST_STATE_PLAYING ret = %d( GST_STATE_CHANGE_ASYNC)", __func__, __LINE__, ret_state);             } 

	mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] Sucess GST_STATE_CHANGE", __func__, __LINE__);

	/* error */
	if (ret_state == 0) 
	{
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] GST_STATE_CHANGE_FAILURE", __func__, __LINE__); //  GST_STATE_CHANGE_SUCCESS = 1,  GST_STATE_CHANGE_ASYNC  = 2,   GST_STATE_CHANGE_NO_PREROLL= 3 
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}
	else
	{
		#if 0
			g_main_loop_run (gstreamer_h->loop);
		#endif
		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] Set GST_STATE_NULL", __func__, __LINE__);
		
		/*GST_STATE_NULL*/
		gst_element_set_state (gstreamer_h->pipeline, GST_STATE_NULL);
		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] End GST_STATE_NULL", __func__, __LINE__);
		
		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] ###gstreamer_h->output_buffer### : %p", __func__, __LINE__, gstreamer_h->output_buffer);
		
		ret_state = gst_element_get_state (gstreamer_h->pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
		
		if(ret_state  == 1)   	{ mmf_debug (MMF_DEBUG_LOG, "[%s][%05d] GST_STATE_NULL ret_state = %d (GST_STATE_CHANGE_SUCCESS)\n", __func__, __LINE__, ret_state);}
		else if( ret_state == 2)  { mmf_debug (MMF_DEBUG_LOG, "[%s][%05d] GST_STATE_NULL ret_state = %d (GST_STATE_CHANGE_ASYNC)\n", __func__, __LINE__, ret_state);}                  

		mmf_debug (MMF_DEBUG_LOG, "[%s][%05d] Success gst_element_get_state\n", __func__, __LINE__);  
		
		if (ret_state == 0) 
		{
			mmf_debug(MMF_DEBUG_ERROR, "GST_STATE_CHANGE_FAILURE"); //  GST_STATE_CHANGE_SUCCESS = 1,  GST_STATE_CHANGE_ASYNC  = 2,   GST_STATE_CHANGE_NO_PREROLL= 3 
		}
		else
		{ 
			if(gstreamer_h->output_buffer != NULL) 
			{
				int buffer_size  = GST_BUFFER_SIZE(gstreamer_h->output_buffer) ;
				mmf_debug (MMF_DEBUG_LOG, "[%s][%05d] buffer size: %d\n", __func__, __LINE__, buffer_size);  
				if( buffer_size !=  mm_setup_image_size(_imgp_info->output_format_label, _imgp_info->output_stride, _imgp_info->output_elevation))
				{					
					mmf_debug (MMF_DEBUG_ERROR, "[%s][%05d] Buffer size is different\n", __func__, __LINE__);  
				}
				mmf_debug (MMF_DEBUG_LOG, "[%s][%05d] gstreamer_h->output_buffer: 0x%2x\n", __func__, __LINE__, gstreamer_h->output_buffer);  
				memcpy( _imgp_info->dst, (char*)GST_BUFFER_DATA(gstreamer_h->output_buffer), buffer_size);		
			}
			else
			{
				mmf_debug (MMF_DEBUG_LOG, "[%s][%05d] gstreamer_h->output_buffer is NULL", __func__, __LINE__); 
			}
		}
		gst_object_unref (gstreamer_h->pipeline);
  		g_free(GST_BUFFER_MALLOCDATA(gstreamer_h->output_buffer)); gstreamer_h->output_buffer = NULL; //gst_buffer_finalize(gstreamer_h->output_buffer) { buffer->free_func (buffer->malloc_data);}
		mmf_debug (MMF_DEBUG_LOG, "[%s][%05d] #g_free# gstreamer_h->output_buffer: 0x%2x\n", __func__, __LINE__, gstreamer_h->output_buffer); 
		
		g_free (gstreamer_h); 
		mmf_debug (MMF_DEBUG_LOG, "[%s][%05d] gstreamer_h: 0x%2x\n", __func__, __LINE__, gstreamer_h); 
		
		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] End mm_convert_colorspace \n", __func__, __LINE__);		
	}		
	mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] _imgp_info->dst: %p", __func__, __LINE__, _imgp_info->dst);	
	return ret;
}		


static int
mm_setup_image_size(const char* _format_label, int width, int height)
{
	int size=0;

	if(strcmp(_format_label, "I420") == 0)		      		{  setup_image_size_I420(width, height); } //width * height *1.5;
	else if(strcmp(_format_label, "Y42B") == 0)			{  setup_image_size_Y42B(width, height); } //width * height *2;	 			
	else if(strcmp(_format_label, "YUV422") == 0)		{  setup_image_size_Y42B(width, height); } //width * height *2;	 			
	else if(strcmp(_format_label, "Y444") == 0)			{  setup_image_size_Y444(width, height); } //width * height *3;
	else if(strcmp(_format_label, "YV12") == 0)			{  setup_image_size_YV12(width, height); } //width * height *1;
	else if(strcmp(_format_label, "NV12") == 0)			{  setup_image_size_NV12(width, height); } //width * height *1.5;			
	else if(strcmp(_format_label, "RGB565") == 0)		{  setup_image_size_RGB565(width, height); } //width * height *2;
	else if(strcmp(_format_label, "RGB888") == 0)		{  setup_image_size_RGB888(width, height); } //width * height *3;
	else if(strcmp(_format_label, "BGR888") == 0)		{  setup_image_size_BGR888(width, height); } //width * height *3;	
	else if(strcmp(_format_label, "UYVY") == 0)			{  setup_image_size_UYVY(width, height); } //width * height *2;	
	else if(strcmp(_format_label, "YUYV") == 0)			{  setup_image_size_YUYV(width, height); } //width * height *2;
	else if(strcmp(_format_label, "ARGB8888") == 0)		{ size = width * height *4; mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] file_size: %d\n", __func__, __LINE__, size);}
	else if(strcmp(_format_label, "BGRA8888") == 0)		{ size = width * height *4; mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] file_size: %d\n", __func__, __LINE__, size);}
	else if(strcmp(_format_label, "RGBA8888") == 0)		{ size = width * height *4; mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] file_size: %d\n", __func__, __LINE__, size);}
	else if(strcmp(_format_label, "ABGR8888") == 0)		{ size = width * height *4; mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] file_size: %d\n", __func__, __LINE__, size);} 
	else if(strcmp(_format_label, "BGRX") == 0)			{ size = width * height *4; mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] file_size: %d\n", __func__, __LINE__, size);}

	return size;
}

static int
_mm_imgp_gstcs(imgp_info* _imgp_info)
{
	image_format* input_format=NULL, *output_format=NULL;
	gstreamer* gstreamer_h;
	int ret = MM_ERROR_NONE;
	g_type_init();
	if(_imgp_info == NULL)
	{
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] imgp_info is NULL", __func__, __LINE__);
	}

	mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] [input] format label : %s width: %d height: %d\t[output] format label: %s width: %d height: %d rotation vaule: %d dst: 0x%2x", __func__, __LINE__,   
   	      _imgp_info->input_format_label,  _imgp_info->src_width, _imgp_info->src_height, _imgp_info->output_format_label,  _imgp_info->dst_width, _imgp_info->dst_height, _imgp_info->angle, _imgp_info->dst);  
     	      
	_imgp_info->output_stride = _imgp_info->dst_width;
	_imgp_info->output_elevation = _imgp_info->dst_height;	
	if(_imgp_info->dst == NULL)
	{
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] imgp_info->dst is NULL", __func__, __LINE__);
	}
	input_format= _mm_set_input_image_format_struct(_imgp_info);	
	output_format= _mm_set_output_image_format_struct(_imgp_info);
	
	mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] mm_check_resize_format&&mm_check_rotate_format ", __func__, __LINE__);
	
	if( __mm_check_resize_format(_imgp_info->input_format_label, _imgp_info->src_width, _imgp_info->src_height, _imgp_info->output_format_label, _imgp_info->output_stride, _imgp_info->output_elevation) 
		&& __mm_check_rotate_format(_imgp_info->angle, _imgp_info->input_format_label, _imgp_info->output_format_label) )
	{
		#if 0 // def GST_EXT_TIME_ANALYSIS
			MMTA_INIT();
		#endif
		gst_init (NULL, NULL);

		gstreamer_h = g_new0 (gstreamer, 1);		

		#if 0 // def GST_EXT_TIME_ANALYSIS
			 MMTA_ACUM_ITEM_BEGIN("ffmpegcolorspace", 0);
		#endif
		/* _format_label : I420, RGB888 etc*/
		mmf_debug(MMF_DEBUG_LOG,"[%s][%05d] Start mm_convert_colorspace ", __func__, __LINE__);
		ret =_mm_imgp_gstcs_processing(gstreamer_h, input_format, output_format, _imgp_info); //input: buffer pointer for input image , input  image format, input image width, input image height, output: buffer porinter for output image

		if(ret == MM_ERROR_NONE)
		{
			mmf_debug(MMF_DEBUG_LOG, "[%s][%05d] End mm_convert_colorspace [_imgp_info->dst: %p]", __func__, __LINE__, _imgp_info->dst);
		}
		else if (ret != MM_ERROR_NONE)
		{
			mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] ERROR -mm_convert_colorspace", __func__, __LINE__);
		}
		#if 0 //def GST_EXT_TIME_ANALYSIS 
			MMTA_ACUM_ITEM_END("ffmpegcolorspace", 0);
			MMTA_ACUM_ITEM_SHOW_RESULT();
			MMTA_ACUM_ITEM_SHOW_RESULT_TO(MMTA_SHOW_FILE);
			MMTA_RELEASE ();
		#endif
	}
	else 
	{
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] Error - Check your input / ouput image input_format_label: %s src_width: %d src_height: %d output_format_label: %s output_stride: %d output_elevation: %d  angle: %d ",__func__, __LINE__,
		_imgp_info->input_format_label, _imgp_info->src_width, _imgp_info->src_height, _imgp_info->output_format_label, _imgp_info->output_stride, _imgp_info->output_elevation, _imgp_info->angle);
		return MM_ERROR_IMAGE_INVALID_VALUE;
	}
	if(input_format) { free(input_format); input_format = NULL; }
	if(output_format) { free(output_format); output_format  = NULL;  }
	return ret;
}

int
mm_imgp(imgp_info* _imgp_info, imgp_type _imgp_type)
{	
	if (_imgp_info == NULL)
	{
		mmf_debug(MMF_DEBUG_ERROR, "[%s][%05d] input vaule is error", __func__, __LINE__);
	}
	return _mm_imgp_gstcs(_imgp_info);
}


