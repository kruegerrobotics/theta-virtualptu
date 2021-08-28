#include <cstdio>
#include <iostream>
#include <thread>
#include <chrono>

#include <gst/gst.h>

#include "ptp.h"
#include "thetacontrol.hpp"

int main()
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

	//now the gestreamer part
	//starting with simple testsrc and autovideosink
	GMainLoop *loop;

	GstElement *pipeline;
	GstElement *source;
	GstElement *sink;
	GstBus *bus;
	guint bus_watch_id;

	/* Initialisation */
	gst_init(&argc, &argv);

	loop = g_main_loop_new(NULL, FALSE);
}

/*
	if (0 == thetaCtrl.disableSleepMode())
	{
		std::cout << "theta is awake" << std::endl;
	}
	else
	{
		std::cout << "could not wake-up theta - aborting" << std::endl;
		return -1;
	}
*/
/*struct usb_device * thetaVDev = NULL;
	thetaVDev= list_devices(false);
	if (thetaVDev != NULL){
		printf("Found a Theta V - checking mode");
	} else {
		printf("No theta V detected ");
		return -1;
	}
ptp_closesession(&params);
*/
