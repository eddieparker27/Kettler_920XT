#include "ant_libusb.h"
#include "checksum.h"
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

#define READ_BUFF_SIZE      (4096)

static UCHAR rx_buff[ READ_BUFF_SIZE ];
static int rx_idx;
static pthread_t tid;
static Input_Handler_Config *ih_cfg = NULL;

static void*
input_handler_thread(void *arg);

int
create_ant_libusb_input_handler(libusb_device_handle *dev_handle,
                                Message_Callback_Fn cb_fn,
                                UCHAR *buffer)
{
   int res = 0;
   ih_cfg = malloc(sizeof(Input_Handler_Config));
   ih_cfg->dev_handle = dev_handle;
   ih_cfg->cb_fn = cb_fn;
   ih_cfg->buffer = buffer;

   res = pthread_create(&tid, NULL, &input_handler_thread, NULL);
   if (res != 0)
   {
      printf("Can't create thread\n");
      return res;
   }

   return res;
}

BOOL
ant_write_message(UCHAR type, UCHAR *data, UCHAR len)
{
   int r;
   int actual;
   BOOL res = FALSE;
   UCHAR buff[ 256 ];
   buff[ 0 ] = MESG_TX_SYNC;
   buff[ 1 ] = len;
   buff[ 2 ] = type;
   memcpy(buff + 3, data, len);
   buff[ 3 + len ] = calc_checksum(buff, (3 + len));

   if (ih_cfg != NULL)
   {
      r = libusb_bulk_transfer(ih_cfg->dev_handle, 
                               (1 | LIBUSB_ENDPOINT_OUT), 
                               buff, 
                               3 + len + 1, 
                               &actual, 
                               0);
      if ((r == 0) && (actual == (3 + len + 1)))
      {
         res = TRUE;
      }
      else
      {
         printf("Writing FAILED! r = %d actual=%d\n", r, actual);
      }
   }
   return res;
}

BOOL
ant_reset_system(void)
{
   UCHAR buff[ 1 ] = { 0 };
   return ant_write_message(MESG_SYSTEM_RESET_ID, buff, 1);
}

BOOL
ant_set_transmit_power(UCHAR lvl)
{
   UCHAR buff[ 2 ] = { 0, lvl };
   return ant_write_message(MESG_CHANNEL_RADIO_TX_POWER_ID, buff, 2);
}

BOOL
ant_set_network_key(UCHAR net_number, UCHAR net_key[ 8 ])
{
   UCHAR buff[ 9 ] = { net_number, net_key[ 0 ], net_key[ 1 ], net_key[ 2 ],
                       net_key[ 3 ], net_key[ 4 ], net_key[ 5 ],
                       net_key[ 6 ], net_key[ 7 ] };
   return ant_write_message(MESG_NETWORK_KEY_ID, buff, 9);
}

BOOL
ant_assign_channel(UCHAR ant_channel, UCHAR channel_type, UCHAR net_number)
{
   UCHAR buff[ 3 ] = { ant_channel, channel_type, net_number };
   return ant_write_message(MESG_ASSIGN_CHANNEL_ID, buff, 3);
}

BOOL
ant_set_channel_id(UCHAR ant_channel, USHORT device_num, UCHAR device_type, UCHAR transmission_type)
{
   UCHAR buff[ 5 ] = { ant_channel, 
                      (device_num & 0xFF), 
                      ((device_num >> 8) & 0xFF),
                      device_type,
                      transmission_type };
   return ant_write_message(MESG_CHANNEL_ID_ID, buff, 5);
}

BOOL
ant_set_channel_rf_freq(UCHAR ant_channel, UCHAR rf_freq)
{
   UCHAR buff[ 2 ] = { ant_channel, rf_freq };
   return ant_write_message(MESG_CHANNEL_RADIO_FREQ_ID, buff, 2);
}

BOOL
ant_open_channel(UCHAR ant_channel)
{
   UCHAR buff[ 1 ] = { ant_channel };
   return ant_write_message(MESG_OPEN_CHANNEL_ID, buff, 1);
}

BOOL
ant_set_channel_period(UCHAR ant_channel, USHORT period)
{
   UCHAR buff[ 3 ] = { ant_channel, 
                      (period & 0xFF), 
                      ((period >> 8) & 0xFF) };
   return ant_write_message(MESG_CHANNEL_MESG_PERIOD_ID, buff, 3);
}

BOOL
ant_send_broadcast_data(UCHAR ant_channel, UCHAR data[ 8 ])
{
   UCHAR buff[ 9 ] = { ant_channel, data[ 0 ], data[ 1 ], data[ 2 ],
                       data[ 3 ], data[ 4 ], data[ 5 ],
                       data[ 6 ], data[ 7 ] };
   return ant_write_message(MESG_BROADCAST_DATA_ID, buff, 9);
}


/* pthread to handle ANT device input */
static void*
input_handler_thread(void *arg)
{
   int actual;
   int idx;
   int len;
   int r;
   
   while(TRUE)
   {
      r = libusb_bulk_transfer(ih_cfg->dev_handle, 
                               (1 | LIBUSB_ENDPOINT_IN), 
                               rx_buff + rx_idx, 
                               READ_BUFF_SIZE - rx_idx, 
                               &actual, 
                               0);
      if (r == 0)
      {
         actual += rx_idx;
         idx = 0;
         while(idx < actual)
         {
            while((idx < actual) && 
                  (rx_buff[ idx ] != MESG_RX_SYNC) && 
                  (rx_buff[ idx ] != MESG_TX_SYNC))
            {
               printf("Skipping byte 0x%x\n", rx_buff[ idx ]);
               idx++;
            }
            if (idx < (actual - 3))
            {
               len = rx_buff[ idx + 1 ];
               if ((idx + len + 4) <= actual)
               {
                  /* Checksum */
                  UCHAR cs = calc_checksum(rx_buff + idx, 3 + len);
                  if (cs == rx_buff[ rx_idx + 3 + len ])
                  {
                     memcpy(ih_cfg->buffer, rx_buff + idx, 4 + len);
                     ih_cfg->cb_fn(ih_cfg->buffer);
                     idx += 4 + len;
                  }
                  else
                  {
                     /* Failed checksum */
                     idx++;
                  }
               }
               else
               {                  
                  rx_idx = 0;
                  while(idx < actual)
                  {
                     rx_buff[ rx_idx++ ] = rx_buff[ idx++ ];
                  }
               }
            }
            else
            {
               rx_idx = 0;
               while(idx < actual)
               {
                  rx_buff[ rx_idx++ ] = rx_buff[ idx++ ];
               }
            }
         }
      }
   }
}




