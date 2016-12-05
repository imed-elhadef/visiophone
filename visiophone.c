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
//******************************************//

void Active_LED_Call(void)
{
 /* export */
  fn = "/sys/class/gpio/export";
  fdled1 = open(fn, O_WRONLY);
  if (fdled1 < 0)
  ERREXIT("open export")

 //Write our value of "26" to the file
  write(fdled1,"26",2);
  close(fdled1);  
  printf("...export file accessed, new pin now accessible\n");

  sleep(1); //---> You must add it for RPI 

 //SET DIRECTION
  //Open the LED's sysfs file in binary for reading and writing, store file pointer in fp
  fn ="/sys/class/gpio/gpio26/direction";
  fdled1 = open(fn,O_RDWR);

  if (fdled1 < 0) 
  ERREXIT("open direction")
  write(fdled1,"out",3);
  close(fdled1);
  printf("...direction set to output\n");
  
//Set Value
  fn="/sys/class/gpio/gpio26/value";
  fdled1 = open(fn,O_RDWR);
  if (fdled1 < 0) 
  ERREXIT("open value")//1:LED ON \ 0:LED OFF
  write(fdled1,"1",1);   
}

void Stop_LED_Call(void)
     {
    write(fdled1,"0",1);//Disable LED
    close(fdled1);
    fdled1=-1;
      }

void Active_LED_Communication(void)
{
 /* export */
  fn = "/sys/class/gpio/export";
  fdled2 = open(fn, O_WRONLY);
  if (fdled2 < 0)
  ERREXIT("open export")

 //Write our value of "12" to the file
  write(fdled2,"12",2);
  close(fdled2);  
  printf("...export file accessed, new pin now accessible\n");

  sleep(1); //---> You must add it for RPI

 //SET DIRECTION
  //Open the LED's sysfs file in binary for reading and writing, store file pointer in fp
  fn ="/sys/class/gpio/gpio12/direction";
  fdled2 = open(fn,O_RDWR);

  if (fdled2 < 0) 
  ERREXIT("open direction")
  write(fdled2,"out",3);
  close(fdled2);
  printf("...direction set to output\n");
  
//Set Value
  fn="/sys/class/gpio/gpio12/value";
  fdled2 = open(fn,O_RDWR);
  if (fdled2 < 0) 
  ERREXIT("open value")//1:LED ON \ 0:LED OFF
  write(fdled2,"1",1);   
}

void Stop_LED_Communication(void)
     {
    write(fdled2,"0",1);//Disable LED
    close(fdled2);
    fdled2=-1;
      }

void Active_LED_Porte(void)
{
 /* export */
  fn = "/sys/class/gpio/export";
  fdled3 = open(fn, O_WRONLY);
  if (fdled3 < 0)
  ERREXIT("open export")

 //Write our value of "5" to the file
  write(fdled3,"5",1);
  close(fdled3);  
  printf("...export file accessed, new pin now accessible\n");

  sleep(1); //---> You must add it for RPI

 //SET DIRECTION
  //Open the LED's sysfs file in binary for reading and writing, store file pointer in fp
  fn ="/sys/class/gpio/gpio5/direction";
  fdled3 = open(fn,O_RDWR);

  if (fdled3 < 0) 
  ERREXIT("open direction")
  write(fdled3,"out",3);
  close(fdled3);
  printf("...direction set to output\n");
  
//Set Value
  fn="/sys/class/gpio/gpio5/value";
  fdled3 = open(fn,O_RDWR);
  if (fdled3 < 0) 
  ERREXIT("open value")//1:LED ON \ 0:LED OFF
  write(fdled3,"1",1);   
}

void Stop_LED_Porte(void)
     {
    write(fdled3,"0",1);//Disable LED
    close(fdled3);
    fdled3=-1;
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



