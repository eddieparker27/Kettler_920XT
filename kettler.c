#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <termios.h>
#include <string.h>
#include "inc/types.h"

#include "time.h"

#define FIELD_PULSE            (0)
#define FIELD_CADENCE          (1)
#define FIELD_SPEED            (2)
#define FIELD_DISTANCE         (3)
#define FIELD_DESIRED_POWER    (4)
#define FIELD_ENERGY           (5)
#define FIELD_TIME             (6)
#define FIELD_ACHIEVED_POWER   (7)

void
power_sensor_set_params(USHORT cum_pow, UCHAR cadence);

void
speed_sensor_set_speed(ULONG speed);



int
main(int argc, char *argv[])
{
   int res;
   int i;

   int USB = -1;

   while(USB < 0)
   {
      USB = open("/dev/ttyUSB1", O_RDWR | O_NONBLOCK | O_NDELAY);

      if (USB < 0)
      {
         printf("Failed to open /dev/ttyUSB1 - error = %d (%s)\n", errno, strerror(errno));
      }
      sleep(5);
   }

   struct termios tty;
   memset(&tty, 0, sizeof(tty));

   if(tcgetattr(USB, &tty) != 0)
   {
      printf("Error %d from tcgetattr: %s\n", errno, strerror(errno));
   }

   cfsetospeed(&tty, B9600);
   cfsetispeed(&tty, B9600);
 
   tty.c_cflag     &=  ~PARENB;
   tty.c_cflag     &=  ~CSTOPB;
   tty.c_cflag     &=  ~CSIZE;
   tty.c_cflag     |=  CS8;
   tty.c_cflag     &=  ~CRTSCTS;
   tty.c_lflag     =  0;
   tty.c_oflag     =  0;
   tty.c_cc[VMIN]  =  0;
   tty.c_cc[VTIME] =  0;

   tty.c_cflag     |=  CREAD | CLOCAL;
   tty.c_iflag     &=  ~(IXON | IXOFF | IXANY);
   tty.c_lflag     &=  ~(ICANON | ECHO | ECHOE | ISIG);
   tty.c_oflag     &=  ~OPOST;

   tcflush(USB, TCIFLUSH);

   if (tcsetattr(USB, TCSANOW, &tty) != 0)
   {
      printf("Error %d from tcsetattr: %s\n", errno, strerror(errno));
   }

   ANT_init();

   char str[ 512 ];
   BOOL running = FALSE;
   int size;
   

   /*
   * Get to running state
   */
   while(!running)
   {
      size = sprintf(str, "CM\r\n");
      res = write(USB, str, size);
      if (res != size)
      {
         printf("Failed to send [CM]\n");
      }
      else
      {
         usleep(1000000);
         res = read(USB, str, 512);
         if (res < 0)
         {
            printf("Failed to read response to [CM]\n");
         }
         else
         {
            printf("Response to [CM] == %s\n", str);
            if (strncmp(str, "RUN", 3) == 0)
            {
               running = TRUE;
            }
            else
            {
               for(i = 0; i < strlen(str); i++)
               {
                  printf("[0x%02x][%c]\n", str[ i ], str[ i ]);
               }
            }
         }
      }
   }

   int pulse;
   int cadence;
   int speed;
   int distance;
   int desired_power;
   int energy;
   char time_string[ 20 ];
   int achieved_power;

   USHORT cum_pow = 0;

   struct timespec tp;
   ULONG last_time;
   ULONG time;
   ULONG elapsed;

   last_time = get_ms();   

   while(TRUE)
   {
      size = sprintf(str, "ST\r\n");
      res = write(USB, str, size);
      if (res != size)
      {
         printf("Failed to send [ST]\n");
      }
      else
      {
         usleep(250000);
         res = read(USB, str, 512);
         if (res < 0)
         {
            printf("Failed to read response to [ST]\n");
         }
         else
         {
            printf(str);
            char *pch;
            pch = strtok(str, "\t");
            int field = 0;
            while(pch != NULL)
            {
               switch(field)
               {
               case FIELD_PULSE:
                  pulse = atoi(pch);
                  break;
               case FIELD_CADENCE:
                  cadence = atoi(pch);
                  break;
               case FIELD_SPEED:
                  speed = atoi(pch);
                  break;
               case FIELD_DISTANCE:
                  distance = atoi(pch);
                  break;
               case FIELD_DESIRED_POWER:
                  desired_power = atoi(pch);
                  break;
               case FIELD_ENERGY:
                  energy = atoi(pch);
                  break;
               case FIELD_TIME:
                  strcpy(time_string, pch);
                  break;
               case FIELD_ACHIEVED_POWER:
                  achieved_power = atoi(pch);
                  break;
               }
           
               pch = strtok(NULL, "\t");
               field++;
            }

            time = get_ms();
            elapsed = time - last_time;
            last_time = time;

            cum_pow += (achieved_power * elapsed) / 1000;

            speed_sensor_set_speed(speed * 100);
            power_sensor_set_params(cum_pow, cadence);
         }
      }
   }
     

   close(USB);   
}

