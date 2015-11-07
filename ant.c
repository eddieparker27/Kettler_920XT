#include <stdio.h>
#include </usr/include/libusb-1.0/libusb.h>
#include "inc/antmessage.h"

#include "ant_libusb.h"
#include "checksum.h"
#include "time.h"

#define ANT_NETWORK_NUMBER              (0)

#define ANT_PLUS_NETWORK_KEY            { 0xB9, 0xA5, \
                                          0x21, 0xFB, \
                                          0xBD, 0x72, \
                                          0xC3, 0x45 }

#define SPEED_SENSOR_ANT_CHANNEL        (0)
#define POWER_SENSOR_ANT_CHANNEL        (1)

#define SPEED_SENSOR_DEVICE_NUMBER      (1)
#define POWER_SENSOR_DEVICE_NUMBER      (3)

#define SPEED_SENSOR_DEVICE_TYPE        (123)
#define POWER_SENSOR_DEVICE_TYPE        (11)

#define SPEED_SENSOR_TRANSMISSION_TYPE  (1)
#define POWER_SENSOR_TRANSMISSION_TYPE  (5)

#define ANT_PLUS_RADIO_FREQ             (57) // 2457 MHz

#define SPEED_SENSOR_CHANNEL_PERIOD     (8118)
#define POWER_SENSOR_CHANNEL_PERIOD     (8182)


static UCHAR tx_buff[ 2 ][ 8 ];

static UCHAR rx_buff[ 256 ];

static void print_devs(libusb_device **devs)
{
   libusb_device *dev;
   int i = 0, j = 0;
   uint8_t path[8]; 

   while ((dev = devs[i++]) != NULL) 
   {
      struct libusb_device_descriptor desc;
      int r = libusb_get_device_descriptor(dev, &desc);
      if (r < 0)
      {
         fprintf(stderr, "failed to get device descriptor");
         return;
      }

      printf("%04x:%04x (bus %d, device %d)",
             desc.idVendor, desc.idProduct,
             libusb_get_bus_number(dev),
             libusb_get_device_address(dev));

      r = libusb_get_port_numbers(dev, path, sizeof(path));
      if (r > 0)
      {
         printf(" path: %d", path[0]);
         for (j = 1; j < r; j++)
         {
            printf(".%d", path[j]);
         }
      }
      printf("\n");
   }
}

void
handle_event(UCHAR ant_channel, UCHAR event)
{
   switch(event)
   {
   case EVENT_TX:
      if (ant_channel == SPEED_SENSOR_ANT_CHANNEL)
      {
         speed_sensor_handle_transmit(tx_buff[ ant_channel ]);
      }
      else if (ant_channel == POWER_SENSOR_ANT_CHANNEL)
      {
         power_sensor_handle_transmit(tx_buff[ ant_channel ]);
      }
      ant_send_broadcast_data(ant_channel, tx_buff[ ant_channel ]);
      break;
   }
}

void
print_response_status(UCHAR status)
{
   switch(status)
   {
   case RESPONSE_NO_ERROR:
      printf("    RESPONSE_NO_ERROR\n");
      break;
   case CHANNEL_IN_WRONG_STATE:
      printf("    CHANNEL_IN_WRONG_STATE\n");
      break;
   default:
      printf("    UNKNWON RESPONSE / EVENT 0x%x\n", status);
      break;   
   } 
}
 

int
process_ANT_message(UCHAR *msg)
{
   UCHAR msg_type = msg[ 2 ];
   switch(msg_type)
   {
   case MESG_STARTUP_MESG_ID:
      printf("*** MESG_STARTUP_MESG_ID\n");
      break;
   case MESG_RESPONSE_EVENT_ID:
      switch(msg[ 4 ])
      {
      case 0x01: /* This is an EVENT */
         handle_event(msg[ 3 ], msg[ 5 ]);
         break;
      case MESG_CHANNEL_RADIO_TX_POWER_ID:
         printf("    RESPONSE TO MESG_CHANNEL_RADIO_TX_POWER_ID\n");
         print_response_status(msg[ 5 ]);                        
         break;
      case MESG_NETWORK_KEY_ID:
         printf("    RESPONSE TO MESG_NETWORK_KEY_ID\n");
         print_response_status(msg[ 5 ]);
         ant_assign_channel(SPEED_SENSOR_ANT_CHANNEL, PARAMETER_TX_NOT_RX, ANT_NETWORK_NUMBER);
         ant_assign_channel(POWER_SENSOR_ANT_CHANNEL, PARAMETER_TX_NOT_RX, ANT_NETWORK_NUMBER);
         break;
      case MESG_ASSIGN_CHANNEL_ID:
         printf("    RESPONSE TO MESG_ASSIGN_CHANNEL_ID\n");
         print_response_status(msg[ 5 ]);
         if (msg[ 3 ] == SPEED_SENSOR_ANT_CHANNEL)
         {
            ant_set_channel_id(SPEED_SENSOR_ANT_CHANNEL, 
                               SPEED_SENSOR_DEVICE_NUMBER,
                               SPEED_SENSOR_DEVICE_TYPE, 
                               SPEED_SENSOR_TRANSMISSION_TYPE);
         }
         else if (msg[ 3 ] == POWER_SENSOR_ANT_CHANNEL)
         {
            ant_set_channel_id(POWER_SENSOR_ANT_CHANNEL, 
                               POWER_SENSOR_DEVICE_NUMBER,
                               POWER_SENSOR_DEVICE_TYPE, 
                               POWER_SENSOR_TRANSMISSION_TYPE);
         }
         break;
      case MESG_CHANNEL_ID_ID:
         printf("    RESPONSE TO MESG_CHANNEL_ID_ID\n");
         print_response_status(msg[ 5 ]);
         ant_set_channel_rf_freq(msg[ 3 ], 
                                 ANT_PLUS_RADIO_FREQ);
         break;
      case MESG_CHANNEL_RADIO_FREQ_ID:
         printf("    RESPONSE TO MESG_CHANNEL_RADIO_FREQ_ID\n");
         print_response_status(msg[ 5 ]);
         ant_open_channel(msg[ 3 ]);
         break;
      case MESG_OPEN_CHANNEL_ID:
         printf("    RESPONSE TO MESG_OPEN_CHANNEL_ID\n");
         print_response_status(msg[ 5 ]);
         if (msg[ 3 ] == SPEED_SENSOR_ANT_CHANNEL)
         {
            ant_set_channel_period(SPEED_SENSOR_ANT_CHANNEL, 
                                   SPEED_SENSOR_CHANNEL_PERIOD);
         }
         else if (msg[ 3 ] == POWER_SENSOR_ANT_CHANNEL)
         {
            ant_set_channel_period(POWER_SENSOR_ANT_CHANNEL, 
                                   POWER_SENSOR_CHANNEL_PERIOD);
         }         
         break;
      case MESG_CHANNEL_MESG_PERIOD_ID:
         printf("    RESPONSE TO MESG_CHANNEL_MESG_PERIOD_ID\n");
         print_response_status(msg[ 5 ]);                        
         break;
      default:
         printf("    RESPONSE TO !!! UNKNOWN MESSAGE TYPE 0x%x !!!\n",
               msg[ 4 ]);
         print_response_status(msg[ 5 ]);                        
      }
      break;
   case MESG_SERIAL_ERROR_ID:
      printf("!!! SERIAL ERROR !!!\n");
      break;
   default:
      printf("!!! UNKNOWN MESSAGE TYPE 0x%x !!!\n", msg[ 2 ]);
      break;                 
   }
}

void
ANT_message_callback(UCHAR *buffer)
{
   process_ANT_message(buffer);
}

void*
speed_sensor_event_thread(void *args);

void*
power_sensor_event_thread(void *args);


int
ANT_init(void)
{
   libusb_device **devs; //pointer  to pointer of device, used to retrieve a list of devices
   libusb_context *ctx = NULL; //a libusb session
   libusb_device_handle *dev_handle;

   int r;
   size_t cnt;
   int actual;
   pthread_t tid;

   r = libusb_init(&ctx);
   if (r < 0)
   {
      printf("Initialise Error %d\n", r);
      return 1;
   }

   printf("libusb initialised\n");

   libusb_set_debug(ctx, 3); // set verbosity level to 3, as suggested in documentation
   cnt = libusb_get_device_list(ctx, &devs);
   if (cnt < 0)
   {
      printf("Error getting device list %d\n", (int)cnt);
      return 1;
   }
 
   printf("Found %d USB devices\n", (int)cnt);

   print_devs(devs);

   libusb_free_device_list(devs, 1);

   dev_handle = libusb_open_device_with_vid_pid(ctx, 0x0fcf, 0x1008);
   if (dev_handle == NULL)
   {
      printf("Cannot open device\n");
      return 1;
   }

   printf("Device Opened\n");

   if (libusb_kernel_driver_active(dev_handle, 0) == 1)
   {
      printf("Kernel Driver Active!\n");
      if (libusb_detach_kernel_driver(dev_handle, 0) == 0)
      {
         printf("Kernel Driver Detached!\n");
      }
   }
      
   r = libusb_claim_interface(dev_handle, 0); // claim interface 0 (the first) of device
   if (r < 0)
   {
      printf("Cannot claim interface!\n");
      return 1;
   }
   printf("Interface claimed!\n");


   create_ant_libusb_input_handler(dev_handle,
                                   &ANT_message_callback,
                                   rx_buff);

   speed_sensor_init();
   power_sensor_init();

   r = pthread_create(&tid, NULL, &speed_sensor_event_thread, NULL);
   if (r != 0)
   {
      printf("Can't create Speed Sensor thread\n");
      return r;
   }

   r = pthread_create(&tid, NULL, &power_sensor_event_thread, NULL);
   if (r != 0)
   {
      printf("Can't create Power Sensor thread\n");
      return r;
   }

   
   ant_reset_system();
   sleep(2); /* Delay after reset */
   ant_set_transmit_power(RADIO_TX_POWER_LVL_3);
   UCHAR net_key[ 8 ] = ANT_PLUS_NETWORK_KEY;
   ant_set_network_key(ANT_NETWORK_NUMBER, net_key);

   return 0;
}

