#include "time.h"
#include <time.h>

ULONG
get_ms(void)
{
   ULONG ms;
   struct timespec tp;
   clock_gettime(CLOCK_REALTIME, &tp);

   ms = tp.tv_sec * 1000;
   ms += tp.tv_nsec / 1000000;

   return ms;
}