
#ifndef PTP_HELPERS_HPP_FJ4T9
#define PTP_HELPERS_HPP_FJ4T9

#include <usb.h>
extern "C"
{
#include <ptp.h>
}

//constants
const int PTPCAM_USB_URB = 2097152;

const int ptpcam_usb_timeout = 5000;

//types
typedef struct _PTP_USB PTP_USB;
struct _PTP_USB
{
    usb_dev_handle *handle;
    int inep;
    int outep;
    int intep;
};

//macros
//TODO: remove macros asap

/* error reporting macro */
#define ERROR(error) fprintf(stderr, "ERROR: " error);

//functions
void close_usb(PTP_USB *ptp_usb, struct usb_device *dev);
void find_endpoints(struct usb_device *dev, int *inep, int *outep, int *intep);
void init_ptp_usb(PTPParams *params, PTP_USB *ptp_usb, struct usb_device *dev);
struct usb_bus *init_usb();
struct usb_device *  list_devices(short force);

static short ptp_check_int(unsigned char *bytes, unsigned int size, void *data);
static short ptp_read_func(unsigned char *bytes, unsigned int size, void *data);
static short ptp_write_func(unsigned char *bytes, unsigned int size, void *data);

void ptpcam_debug(void *data, const char *format, va_list args);
void ptpcam_error(void *data, const char *format, va_list args);

#endif