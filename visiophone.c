/****************************************************************************/
 //   copyright            : (C) by 2016 Imed Elhadef <imed.elhadef@arcangel.fr>
                               
  
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "visiophone.h"

int press=0;
char fn_led[34];
int fdledcam=-1;//File descriptor of led camera
int fdbutton=-1;//File descriptor of led camera
//******************************************//
void active_led (led_visio *led)
{
 /* export */
  strcpy(fn_led,"/sys/class/gpio/export");
  led->fd = open(fn_led, O_WRONLY);
  if (led->fd < 0)
  ERREXIT("open export")

 //Write our value of "26" to the file
  write(led->fd,led->pin_nbr,2);
  close(led->pin_nbr);  
  printf("...export file accessed, new pin now accessible\n");

  sleep(1); //---> You must add it for RPI 

 //SET DIRECTION
  //Open the LED's sysfs file in binary for reading and writing, store file pointer in fp
  //fn ="/sys/class/gpio/gpio26/direction";
  printf("/sys/class/gpio/gpio%s/direction\n",led->pin_nbr);
  snprintf(fn_led,sizeof(fn_led),"/sys/class/gpio/gpio%s/direction",led->pin_nbr);
  led->fd = open(fn_led,O_RDWR);

  if (led->fd < 0) 
  ERREXIT("open direction")
  write(led->fd,"out",3);
  close(led->fd);
  printf("...direction set to output\n");
  
//Set Value
  printf("/sys/class/gpio/gpio%s/value\n",led->pin_nbr);
  snprintf(fn_led,sizeof(fn_led),"/sys/class/gpio/gpio%s/value",led->pin_nbr);
  //fn_led="/sys/class/gpio/gpio26/value";
  led->fd = open(fn_led,O_RDWR);
  if (led->fd < 0) 
  ERREXIT("open value")//1:LED ON \ 0:LED OFF
  write(led->fd,"1",1);   
}

void stop_led(led_visio *led)
    {
    write(led->fd,"0",1);//Disable LED
    close(led->fd);
    led->fd=-1;
     }


void active_led_camera(void)
{
 /* export */
  fn = "/sys/class/gpio/export";
  fdledcam = open(fn, O_WRONLY);
  if (fdledcam < 0)
  ERREXIT("open export")

 //Write our value of "6" to the file
  write(fdledcam,"6",2);
  close(fdledcam);  
  printf("...export file accessed, new pin now accessible\n");

  sleep(1); //---> You must add it for RPI 

 //SET DIRECTION
  //Open the LED's sysfs file in binary for reading and writing, store file pointer in fp
  fn ="/sys/class/gpio/gpio6/direction";
  fdledcam = open(fn,O_RDWR);

  if (fdledcam < 0) 
  ERREXIT("open direction")
  write(fdledcam,"out",3);
  close(fdledcam);
  printf("...direction set to output\n");
  
//Set Value
  fn="/sys/class/gpio/gpio6/value";
  fdledcam = open(fn,O_RDWR);
  if (fdledcam < 0) 
  ERREXIT("open value")//1:LED ON \ 0:LED OFF
  write(fdledcam,"1",1);   
}

void stop_led_camera(void)
   {
    write(fdledcam,"0",1);//Disable LED
    close(fdledcam);
    fdledcam=-1;
   }


void Init_Polling_Button(void) //Raspberry Pi pin 16 for call button
{
  /* export GPIO 16 */  
  fn = "/sys/class/gpio/export";
  fdbutton = open(fn, O_WRONLY); if(fdbutton < 0)  ERREXIT("open export")
  rc = write(fdbutton, "16", 3); if(rc != 3) ERREXIT("write export")//Active-low button - Broadcom pin 16, P1 pin 36
  close(fdbutton);

  sleep(1); //---> You must add it for RPI
  /* direction */
  
  fn = "/sys/class/gpio/gpio16/direction";//16
  fdbutton = open(fn, O_RDWR);  if(fdbutton < 0)  ERREXIT("open direction")
  rc = write(fdbutton, "in", 3);if(rc != 3) ERREXIT("write direction")
  close(fdbutton);
  
  /* edge */

  fn = "/sys/class/gpio/gpio16/edge";//16
  fdbutton = open(fn, O_RDWR);        if(fdbutton < 0)  ERREXIT("open edge")
  rc = write(fdbutton, "falling", 8); if(rc != 8) ERREXIT("write edge")
  rc = lseek(fdbutton, 0, SEEK_SET);  if(rc < 0)  ERREXIT("lseek edge")
  rc = read(fdbutton, buf_Poll, 10);       if(rc <= 0) ERREXIT("read edge")
  buf_Poll[10] = '\0';
  printf("read gpio16/edge:%s\n", buf_Poll);//16
  close(fdbutton);

  /* wait for interrupt - try it a few times */
  fn = "/sys/class/gpio/gpio16/value";//16
  fdbutton = open(fn, O_RDWR);  if(fdbutton < 0)  ERREXIT("open value")
  xfds[0].fd       = fdbutton;
  xfds[0].events   = POLLPRI | POLLERR;
  xfds[0].revents  = 0;

 }

void Polling_Button (void)
 {     
        //printf("Waiting for interrupts!!!\n");  
  	rc = poll(xfds, 1, 500); if(rc == -1) ERREXIT("poll value") //500 polling time
	rc = lseek(fdbutton, 0, SEEK_SET); if (rc < 0)  ERREXIT("lseek value")
  	rc = read(fdbutton, buf_Poll, 2); if (rc != 2) ERREXIT("read value")
  	buf_Poll[1] = '\0'; // Overwrite the newline character with terminator
    
       if (buf_Poll[0]==49)
        press=0;
}
 
 void Unexport_Polling_Button (void)
{
  /* unexport GPIO 16*/ 
  fn = "/sys/class/gpio/unexport";
  fdbutton = open(fn, O_WRONLY); if(fdbutton < 0)  ERREXIT("open unexport")
  rc = write(fdbutton, "16", 3); if(rc != 3) ERREXIT("write unexport")//16
  close(fdbutton);
 }



