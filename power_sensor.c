#include "inc/types.h"
#include <stdio.h>
#include <pthread.h>

static UCHAR event_count = 0;

static pthread_mutex_t power_mutex;
static pthread_mutex_t event_mutex;

static USHORT cumulative_power;
static UCHAR current_cadence;

static USHORT cum_pow_tx;
static UCHAR cadence_tx;

void
power_sensor_set_params(USHORT cum_pow, UCHAR cadence)
{
   pthread_mutex_lock(&power_mutex);
   cumulative_power = cum_pow;
   current_cadence = cadence;
   pthread_mutex_unlock(&power_mutex);
}

void
power_sensor_init(void)
{
   pthread_mutex_init(&power_mutex, NULL);
   pthread_mutex_init(&event_mutex, NULL);
}

void*
power_sensor_event_thread(void *args)
{
   while(TRUE)
   {
      pthread_mutex_lock(&power_mutex);
      USHORT cum_pow = cumulative_power;
      UCHAR cadence = current_cadence;
      pthread_mutex_unlock(&power_mutex);

      pthread_mutex_lock(&event_mutex);
      ++event_count;

      cum_pow_tx = cum_pow;
      cadence_tx = cadence;

      pthread_mutex_unlock(&event_mutex);

      usleep(1000000);
   }
}

void
power_sensor_handle_transmit(UCHAR *txbuff)
{
   txbuff[ 0 ] = 0x10;

   pthread_mutex_lock(&event_mutex);

   txbuff[ 1 ] = event_count;
   txbuff[ 2 ] = 0xFF;
   txbuff[ 3 ] = cadence_tx;
   txbuff[ 4 ] = (UCHAR) (cum_pow_tx & 0xFF);
   txbuff[ 5 ] = (UCHAR) ((cum_pow_tx >> 8) & 0xFF);
   txbuff[ 6 ] = 0xFF;
   txbuff[ 7 ] = 0xFF;

   pthread_mutex_unlock(&event_mutex);

}
