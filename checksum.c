#include "checksum.h"


UCHAR
calc_checksum(const UCHAR *data, int size)
{
   UCHAR cs = 0;
   int i;
   for(i = 0; i < size; i++)
   {
      cs ^= data[ i ];
   }
   return cs;
}

