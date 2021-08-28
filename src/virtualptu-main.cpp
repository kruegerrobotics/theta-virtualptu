#include <cstdio>
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>

#include <linux/joystick.h>
#include <fcntl.h>

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

#include "libuvc/libuvc.h"
#include "thetauvc.h"

#include "ptp.h"
#include "thetacontrol.hpp"

//global for thread control

bool abortFlag = false;

//datastructure to store data between gstreamer and callbacks
struct GSTData
{
	GstElement *appsrc;
	GTimer *timer;
	uint32_t dwFrameInterval;
};

//gstreamer message loop
static gboolean busCallback(GstBus *bus,
							GstMessage *msg,
							gpointer data);

//call back for every frame on the theta
void thetaFrameCallback(uvc_frame_t *frame, void *ptr);

//handling the joystick
void joystickThreadBody(bool &abortFlag, GstElement *videoCrop);

int main(int argc, char *argv[])
{
	std::cout << "Starting virtual ptu" << std::endl;
	ThetaControl thetaCtrl("RICOH THETA V");

	std::cout << "Scanning usb connections to find the ricoh Theta V" << std::endl;
	if (thetaCtrl.detectFirstThetaV())
	{
		std::cout << "theta found" << std::endl;
	}
	else
	{
		std::cout << "Error: no theta found - aborting" << std::endl;
		return -1;
	}

	std::cout << "check if theta is sleeping" << std::endl;
	const int maxRetries = 3;
	int loopCount = 1;
	while (thetaCtrl.isSleeping())
	{
		if (loopCount <= maxRetries)
		{
			std::cout << "theta is in sleep mode - initiate wake up - attempt: " << loopCount << "/" << maxRetries << std::endl;
			thetaCtrl.wakeUp();
			loopCount++;
		}
		else
		{
			std::cout << "Error: theta can not be woken up - aborting" << std::endl;
			return -1;
		}
	}
	std::cout << "theta is not sleeping" << std::endl;

	std::cout << "check if theta is in streaming mode" << std::endl;
	loopCount = 1;

	//this is dangerous when the device switches modes the USB connetion and id changes
	//device changes from usb, id and type
	while (!thetaCtrl.isInStreamingMode())
	{
		if (loopCount <= maxRetries)
		{
			std::cout << "theta is not in streaming mode - initiating mode switch - attempt: " << loopCount << "/" << maxRetries << std::endl;
			thetaCtrl.switchToStreamingMode();
			loopCount++;
		}
		else
		{
			std::cout << "Error: theta is not ready to stream" << std::endl;
			return -1;
		}
	}
	std::cout << "theta is ready to stream" << std::endl;

	//libuvc stuff
	uvc_context_t *ctx;
	uvc_device_t *dev;
	uvc_device_t **devlist;
	uvc_device_handle_t *devh;
	uvc_stream_ctrl_t ctrl;
	uvc_error_t res;

	std::cout << "connecting to theta via uvc interface" << std::endl;

	//device is in the right mode so we need to connect via
	res = uvc_init(&ctx, NULL);
	if (res != UVC_SUCCESS)
	{
		uvc_perror(res, "uvc_init");
		return -1;
	}

	//src.framecount = 0;
	res = thetauvc_find_device(ctx, &dev, 0);
	if (res != UVC_SUCCESS)
	{
		fprintf(stderr, "THETA not found\n");
		uvc_exit(ctx);
		return -1;
	}

	res = uvc_open(dev, &devh);
	if (res != UVC_SUCCESS)
	{
		fprintf(stderr, "Can't open THETA\n");
		uvc_exit(ctx);
		return -1;
	}

	std::cout << "retrieving theta streaming format" << std::endl;
	res = thetauvc_get_stream_ctrl_format_size(devh,
											   THETAUVC_MODE_UHD_2997, &ctrl);
	if (res != UVC_SUCCESS)
	{
		fprintf(stderr, "Can not retrieve stream format\n");
		uvc_close(devh);
		uvc_exit(ctx);
		return -1;
	}

	//now the gestreamer part
	//starting with simple testsrc and autovideosink
	GMainLoop *loop;

	GstElement *pipeline;
	GstElement *appsource;
	GstElement *queue1;
	GstElement *h264parse;
	GstElement *queue2;
	GstElement *decoder;
	GstElement *videocrop;
	GstElement *sink;
	GstBus *bus;
	guint bus_watch_id;

	gst_init(&argc, &argv);

	loop = g_main_loop_new(NULL, FALSE);
	pipeline = gst_pipeline_new("ptu-stream");
	if (!pipeline)
	{
		std::cout << "Error: can not create gstreamer pipeline" << std::endl;
		return -1;
	}

	appsource = gst_element_factory_make("appsrc", "app-source");
	if (!appsource)
	{
		std::cout << "Error: can not create gstreamer source" << std::endl;
		return -1;
	}

	queue1 = gst_element_factory_make("queue", "queue1");
	if (!queue1)
	{
		std::cout << "Error: can not create gstreamer queue1" << std::endl;
		return -1;
	}

	h264parse = gst_element_factory_make("h264parse", "h264parse");
	if (!h264parse)
	{
		std::cout << "Error: can not create gstreamer h264parse" << std::endl;
		return -1;
	}

	queue2 = gst_element_factory_make("queue", "queue2");
	if (!queue2)
	{
		std::cout << "Error: can not create gstreamer queue2" << std::endl;
		return -1;
	}

	decoder = gst_element_factory_make("avdec_h264", "decoder");
	if (!decoder)
	{
		std::cout << "Error: can not create gstreamer decoder" << std::endl;
		return -1;
	}

	videocrop = gst_element_factory_make("videocrop", "videocrop");
	if (!sink)
	{
		std::cout << "Error: can not create gstreamer videocrop" << std::endl;
		return -1;
	}

	sink = gst_element_factory_make("autovideosink", "display");
	if (!sink)
	{
		std::cout << "Error: can not create gstreamer sink" << std::endl;
		return -1;
	}

	//adding elements
	gst_bin_add(GST_BIN(pipeline), appsource);
	gst_bin_add(GST_BIN(pipeline), queue1);
	gst_bin_add(GST_BIN(pipeline), h264parse);
	gst_bin_add(GST_BIN(pipeline), queue2);
	gst_bin_add(GST_BIN(pipeline), decoder);
	gst_bin_add(GST_BIN(pipeline), videocrop);
	gst_bin_add(GST_BIN(pipeline), sink);

	//linking elements
	gst_element_link(appsource, queue1);
	gst_element_link(queue1, h264parse);
	gst_element_link(h264parse, queue2);
	gst_element_link(queue2, decoder);
	gst_element_link(decoder, videocrop);
	gst_element_link(videocrop, sink);

	const gint totalWidth = 3840;
	const gint totalHeight = 1920;
	const gint width = 640;
	const gint height = 480;
	gint xOffset = 0;
	gint yOffset = 0;

	//calculate right
	gint right = totalWidth - width - xOffset;
	gint bottom = totalHeight - height - yOffset;

	g_object_set(videocrop, "left", xOffset, NULL);
	g_object_set(videocrop, "top", yOffset, NULL);
	g_object_set(videocrop, "right", right, NULL);
	g_object_set(videocrop, "bottom", bottom, NULL);

	g_object_set (sink, "sync", FALSE, NULL);
	
	//capsfilter
	GstCaps *caps = gst_caps_new_simple("video/x-h264",
										"framerate", GST_TYPE_FRACTION, 30000, 1001,
										"stream-format", G_TYPE_STRING, "byte-stream",
										"profile", G_TYPE_STRING, "constrained-baseline", NULL);
	gst_app_src_set_caps(GST_APP_SRC(appsource), caps);

	//the message loop
	bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
	bus_watch_id = gst_bus_add_watch(bus, busCallback, loop);
	gst_object_unref(bus);

	std::cout << "Set pipeline to play" << std::endl;
	gst_element_set_state(pipeline, GST_STATE_PLAYING);

	std::cout << "staring theta live streaming" << std::endl;
	//puttung all information to the data structure
	GSTData callbackData;
	callbackData.dwFrameInterval = ctrl.dwFrameInterval;
	callbackData.appsrc = appsource;
	callbackData.timer = g_timer_new();
	gst_pipeline_set_clock(GST_PIPELINE(pipeline), gst_system_clock_obtain());

	res = uvc_start_streaming(devh, &ctrl, thetaFrameCallback, &callbackData, 0);
	if (res == UVC_SUCCESS)
	{
	}
	else
	{
		uvc_close(devh);
		uvc_exit(ctx);
	}

	//joystick thread
	std::cout << "Startin joystick thread and handling" << std::endl;
	auto joystickThread = std::thread(joystickThreadBody, std::ref(abortFlag), videocrop);

	//holding the main loop
	std::cout << "Pipeline running" << std::endl;
	g_main_loop_run(loop);

	std::cout << "Stopping playback" << std::endl;
	gst_element_set_state(pipeline, GST_STATE_NULL);

	//abortFlag for the joystick thread
	abortFlag = true;
	joystickThread.join();

	//clean up ucv theta
	std::cout << "cleaning uvc connection" << std::endl;
	uvc_close(devh);
	uvc_exit(ctx);

	//clean up gstreamer
	std::cout << "cleaning pipeline" << std::endl;
	gst_object_unref(GST_OBJECT(pipeline));
	g_source_remove(bus_watch_id);
	g_main_loop_unref(loop);
}

static gboolean busCallback(GstBus *bus,
							GstMessage *msg,
							gpointer data)
{
	GMainLoop *loop = (GMainLoop *)data;

	switch (GST_MESSAGE_TYPE(msg))
	{

	case GST_MESSAGE_EOS:
		g_print("End of stream\n");
		g_main_loop_quit(loop);
		break;

	case GST_MESSAGE_ERROR:
	{
		gchar *debug;
		GError *error;

		gst_message_parse_error(msg, &error, &debug);
		g_free(debug);

		g_printerr("Error: %s\n", error->message);
		g_error_free(error);

		g_main_loop_quit(loop);
		break;
	}
	default:
		break;
	}

	return TRUE;
}

void thetaFrameCallback(uvc_frame_t *frame, void *ptr)
{
	GSTData *s;
	GstBuffer *buffer;
	GstFlowReturn ret;
	GstMapInfo map;
	gdouble ms;
	uint32_t pts;

	s = (GSTData *)ptr;
	ms = g_timer_elapsed(s->timer, NULL);

	buffer = gst_buffer_new_allocate(NULL, frame->data_bytes, NULL);
	GST_BUFFER_PTS(buffer) = frame->sequence * s->dwFrameInterval * 100;
	GST_BUFFER_DTS(buffer) = GST_CLOCK_TIME_NONE;
	GST_BUFFER_DURATION(buffer) = s->dwFrameInterval * 100;
	GST_BUFFER_OFFSET(buffer) = frame->sequence;

	gst_buffer_map(buffer, &map, GST_MAP_WRITE);
	memcpy(map.data, frame->data, frame->data_bytes);
	gst_buffer_unmap(buffer, &map);

	g_signal_emit_by_name(s->appsrc, "push-buffer", buffer, &ret);
	gst_buffer_unref(buffer);

	if (ret != GST_FLOW_OK)
	{
		std::cout << "pushbuffer error" << std::endl;
	}
	return;
}

/* Process keyboard input */
static gboolean
handle_keyboard(GIOChannel *source, GIOCondition cond, void *data)
{
	/*
	const gint totalWidth=3840;
	const gint totalHeight=1920;
	const gint width=640;
	const gint height=480;
	gint xOffset = 200;
	gint yOffset = 200;

	//calculate right
	gint right = totalWidth - width - xOffset;
	gint bottom = totalHeight - height - yOffset;

	g_object_set (videocrop, "left", xOffset, NULL);
	g_object_set (videocrop, "top", yOffset, NULL);
	g_object_set (videocrop, "right", right, NULL);
	g_object_set (videocrop, "bottom", bottom, NULL);*/
	gchar *str = NULL;

	if (g_io_channel_read_line(source, &str, NULL, NULL,
							   NULL) != G_IO_STATUS_NORMAL)
	{
		return TRUE;
	}

	switch (g_ascii_tolower(str[0]))
	{

	case 'q':
		std::cout << "q" << std::endl;
		break;
	default:
		break;
	}
	return TRUE;
}

int read_event(int fd, struct js_event *event)
{
	ssize_t bytes;

	bytes = read(fd, event, sizeof(*event));

	if (bytes == sizeof(*event))
		return 0;

	/* Error, could not read full event. */
	return -1;
}

struct axis_state
{
	short x, y;
};

size_t get_axis_state(struct js_event *event, struct axis_state axes[3])
{
	size_t axis = event->number / 2;

	if (axis < 3)
	{
		if (event->number % 2 == 0)
			axes[axis].x = event->value;
		else
			axes[axis].y = event->value;
	}

	return axis;
}

int clamp(int n, int lower, int upper)
{
	return std::max(lower, std::min(n, upper));
}

void joystickThreadBody(bool &abortFlag, GstElement *videoCrop)
{
	std::cout << "joystick thread started" << std::endl;
	const char *device = "/dev/input/js0";
	struct js_event event;
	struct axis_state axes[3] = {0};
	size_t axis;
	int js = open(device, O_RDONLY);
	timeval timeout;

	if (js == -1)
	{
		perror("Could not open joystick");
		return;
	}

	fd_set set;

	uint timeOutCount = 0;
	int xvel = 0;
	int yvel = 0;
	int xOffset = 0;
	int yOffset = 0;
	const gint totalWidth = 3840;
	const gint totalHeight = 1920;
	const gint width = 640;
	const gint height = 480;

	while (!abortFlag)
	{
		timeout.tv_sec = 0;
		timeout.tv_usec = 16000;
		FD_ZERO(&set);	  /* clear the set */
		FD_SET(js, &set); /* add our file descriptor to the set */
		int rv = select(js + 1, &set, NULL, NULL, &timeout);

		if (rv == -1)
		{
			perror("select"); /* an error accured */
		}
		else if (rv == 0)
		{
			//std::cout << "timeout no: " << timeOutCount << std::endl; /* a timeout occured */
			timeOutCount++;
		}
		else
		{
			int bytes = read(js, &event, sizeof(event));
			if (bytes != sizeof(event))
			{
				continue;
			}
			else
			{
				switch (event.type)
				{
				case JS_EVENT_BUTTON:
					//printf("Button %u %s\n", event.number, event.value ? "pressed" : "released");
					break;
				case JS_EVENT_AXIS:
					axis = get_axis_state(&event, axes);
					if (axis < 3)
					{
						//printf("Axis %zu at (%6d, %6d)\n", axis, axes[axis].x, axes[axis].y);
						xvel = axes[0].x / 5000;
						yvel = axes[0].y / 5000;
						//std::cout << "xvel: " << xvel << " yvel: " << yvel << std::endl;
					}
					break;
				default:
					/* Ignore init events. */
					break;
				}
				fflush(stdout);
				std::this_thread::sleep_for(std::chrono::milliseconds(16));
			}
		}

		//calculate the positions
		xOffset += xvel;
		yOffset -= yvel;
		xOffset = clamp(xOffset, 0, totalWidth - width);
		yOffset = clamp(yOffset, 0, totalHeight - height);
		//std::cout << "x: " << xOffset << " y: " << yOffset << std::endl;
		gint right = totalWidth - width - xOffset;
		gint bottom = totalHeight - height - yOffset;

		g_object_set(videoCrop, "left", xOffset, NULL);
		g_object_set(videoCrop, "top", yOffset, NULL);
		g_object_set(videoCrop, "right", right, NULL);
		g_object_set(videoCrop, "bottom", bottom, NULL);
	}
	close(js);
	std::cout << "joystick thread terminated" << std::endl;
}