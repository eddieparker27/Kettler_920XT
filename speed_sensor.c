#include "inc/types.h"
#include <stdio.h>
#include <pthread.h>

#define BS_TOGGLE_MASK         65

static ULONG current_speed;
static pthread_mutex_t speed_mutex;
static pthread_mutex_t event_mutex;
static UCHAR page_toggle = 0;
static USHORT event_count = 0;
static USHORT time_1024 = 0;
static ULONG runtime_16000 = 0;

void
speed_sensor_set_speed(ULONG speed)
{
   pthread_mutex_lock(&speed_mutex);
   current_speed = speed;
   pthread_mutex_unlock(&speed_mutex);
}

void
speed_sensor_init(void)
{
   pthread_mutex_init(&speed_mutex, NULL);
   pthread_mutex_init(&event_mutex, NULL);
}

void*
speed_sensor_event_thread(void *args)
{
   ULONG last_event_time = get_ms();
   ULONG next_event_time = get_ms();
   ULONG timer_interval;

   while(TRUE)
   {
      ULONG time = get_ms();
      if (time >= next_event_time)
      {
         timer_interval = next_event_time - last_event_time;
         pthread_mutex_lock(&event_mutex);
         ++event_count;

         runtime_16000 += timer_interval << 4;
         time_1024 = (USHORT) ((runtime_16000 << 3) / 125);

         pthread_mutex_unlock(&event_mutex);
         last_event_time = next_event_time;
      }

      pthread_mutex_lock(&speed_mutex);
      ULONG speed = current_speed;
      pthread_mutex_unlock(&speed_mutex);

      timer_interval = 1000;
      if (speed)
      {
         timer_interval = ((ULONG) 3600 * 2096 / speed);
         next_event_time = last_event_time + timer_interval;
      }
      if (timer_interval > 1000)
      {
         timer_interval = 1000;
      }
      usleep(timer_interval * 1000);
   }
}

void
speed_sensor_handle_transmit(UCHAR *txbuff)
{
   txbuff[ 0 ] = 0;
   txbuff[ 1 ] = 0xFF;
   txbuff[ 2 ] = 0xFF;
   txbuff[ 3 ] = 0xFF;

   pthread_mutex_lock(&event_mutex);

   txbuff[ 4 ] = (UCHAR) (time_1024 & 0xFF);
   txbuff[ 5 ] = (UCHAR) ((time_1024 >> 8) & 0xFF);
   txbuff[ 6 ] = (UCHAR) (event_count & 0xFF);
   txbuff[ 7 ] = (UCHAR) ((event_count >> 8) & 0xFF);

   pthread_mutex_unlock(&event_mutex);

   page_toggle += 0x20;
   txbuff[ 0 ] += (page_toggle & BS_TOGGLE_MASK);
}
