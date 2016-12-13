/****************************************************************************/
 // copyright: (C) by 2016 Imed Elhadef "Arcangel Technologies" <imed.elhadef@arcangel.fr>
                               
  
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "visiophone.h"
#include "database.h"

#define XBEE_OFFSET 15
//--------------ZigBee Variables-------------//
int zigbee_fd=-1;
const char* serial_port= "/dev/ttyAMA0";//Serial Port for raspberry
//----------------------------------------//
bool interrupt=true;
int press=0;
int fdbutton=-1;//File descriptor of call button
led_visio led_call = {.fd=-1,.pin_nbr="26",.fn_led=""};// Led call infos
led_visio led_communication = {.fd=-1,.pin_nbr="12",.fn_led=""};// Led communication infos
led_visio led_cam = {.fd=-1,.pin_nbr="6",.fn_led=""};// Led camera infos
led_visio led_door = {.fd=-1,.pin_nbr="5",.fn_led=""};// Led door infos
//******************************************//
void active_led (led_visio *led)
{
 /* export */
  strcpy(led->fn_led,"/sys/class/gpio/export");
  led->fd = open(led->fn_led, O_WRONLY);
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
  snprintf(led->fn_led,sizeof(led->fn_led),"/sys/class/gpio/gpio%s/direction",led->pin_nbr);
  led->fd = open(led->fn_led,O_RDWR);

  if (led->fd < 0) 
  ERREXIT("open direction")
  write(led->fd,"out",3);
  close(led->fd);
  printf("...direction set to output\n");
  
//Set Value
  printf("/sys/class/gpio/gpio%s/value\n",led->pin_nbr);
  snprintf(led->fn_led,sizeof(led->fn_led),"/sys/class/gpio/gpio%s/value",led->pin_nbr);
  //led->fn_led="/sys/class/gpio/gpio26/value";
  led->fd = open(led->fn_led,O_RDWR);
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
  	//rc = poll(xfds, 1, 500); if(rc == -1) ERREXIT("poll value") //500 polling time
        rc = poll(xfds, 1, 10); if(rc == -1) ERREXIT("poll value") //10 polling time
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

//********************XBee Module*****************//
//initilize ttyAMA0 dev for XBee
int init_uart_port()
{
 struct termios options;

 zigbee_fd = open(serial_port, O_RDWR | O_NOCTTY | O_NDELAY);
  if (zigbee_fd == -1) {
    perror("open_port: Unable to open /dev/ttyAMA0 - ");
    return 0;
  }

  //set attr 
  tcgetattr(zigbee_fd, &options);
  options.c_cflag = B9600 | CS8 | CLOCAL | CREAD;		//<Set baud rate to 9600
  options.c_iflag = IGNPAR;
  options.c_oflag = 0;
  options.c_lflag = 0;
  tcflush(zigbee_fd, TCIFLUSH);
  tcsetattr(zigbee_fd, TCSANOW, &options);
 
  //Make the UART file discreptor interruptable with sigaction
   fcntl(zigbee_fd, F_SETFL, FNDELAY);
   fcntl(zigbee_fd, F_SETOWN, getpid()); 
   fcntl(zigbee_fd, F_SETFL,  O_ASYNC ); 
  // Turn off blocking for reads, use (fd, F_SETFL, FNDELAY) if you want that
  /*if (fcntl(zigbee_fd, F_SETFL, O_NONBLOCK) < 0)
	{
	 perror("Unable set to NONBLOCK mode");
	 return 0;
	}*/ 

 return 1;
}

//send data to XBee 
int send_uart_data(char* pdata, int size)
{
  interrupt=false;
// Write to the port
  int n = write(zigbee_fd,pdata,size);
  if (n < 0) {
    perror("Write failed - ");
    return 1;
  }
return 0;
}
//receive data from XBee 
int recieve_uart_data(char* pdata, int size)
{
  interrupt=true;
  //read from port
  int n = read(zigbee_fd, (void*)pdata, size);
  if (n < 0) 
  {
    perror("Read failed - ");
    return 1;
  } 
  else if (n == 0) printf("No data on port\n");

  else 
  {
    pdata[n] = '\0';
    printf("%i bytes read : %s\n", n, pdata+XBEE_OFFSET);
  }
return 0;
}

void zigbee_handle (door_visio *d)
    {
        recieve_uart_data(d->packet_from_zigbee,2);            
        if (!strcmp(d->packet_from_zigbee,"OK"))//Porte Ouverte               
         {             
           active_led(&led_door);//Activate the open door led
           porte_ouverte();//Changing the door variable in database
           system("aplay -q /home/pi/Porte_Ouverte.wav");
         }

       if (!strcmp(d->packet_from_zigbee,"FD"))//Porte Fermee
        {  
          stop_led(&led_door);//Desactivate the open door led
          porte_fermee();//Changing the  door variable in database
          system("aplay -q /home/pi/Porte_Fermee.wav");                        
         }       
       if (!strcmp(d->packet_from_zigbee,"DF"))//Porte Forcee                                     
        {  
          porte_forcee();//Changing the door variable in database
          system("aplay -q /home/pi/Porte_Forcee.wav");                       
         }           
  
    }
