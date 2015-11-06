#ifndef ANT_LIBUSB_H
#define ANT_LIBUSB_H

#include </usr/include/libusb-1.0/libusb.h>
#include "inc/antmessage.h"

typedef void (*Message_Callback_Fn)(UCHAR *buffer);

typedef struct
{
   libusb_device_handle *dev_handle;
   Message_Callback_Fn cb_fn;
   UCHAR *buffer;
} Input_Handler_Config;

int
create_ant_libusb_input_handler(libusb_device_handle *dev_handle,
                                Message_Callback_Fn cb_fn,
                                UCHAR *buffer);

BOOL
ant_reset_system(void);

BOOL
ant_set_transmit_power(UCHAR lvl);

BOOL
ant_set_network_key(UCHAR net_number, UCHAR net_key[ 8 ]);

BOOL
ant_assign_channel(UCHAR ant_channel, UCHAR channel_type, UCHAR net_number);

BOOL
ant_set_channel_id(UCHAR ant_channel, USHORT device_num, UCHAR device_type, UCHAR transmission_type);

BOOL
ant_set_channel_rf_freq(UCHAR ant_channel, UCHAR rf_freq);

BOOL
ant_open_channel(UCHAR ant_channel);

BOOL
ant_set_channel_period(UCHAR ant_channel, USHORT period);

BOOL
ant_send_broadcast_data(UCHAR ant_channel, UCHAR data[ 8 ]);

#endif

