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

const int ledcall = 26; // Regular LED - Broadcom pin 26
const int ledcommun = 12; // Regular LED - Broadcom pin 12
const int ledcam = 6; // Regular LED - Broadcom pin 6
const int leddoor = 5; // Regular LED - Broadcom pin 5
const int callbutton = 16; // Active-low Capacitive button - Broadcom pin 16, P1 pin 36

//******************************************//
void init_visiophone_leds(void)
{
    pinMode(ledcall, OUTPUT);       // Set regular call LED as output
    pinMode(ledcommun, OUTPUT);     // Set regular communication LED as output
    pinMode(ledcam, OUTPUT);       // Set regular camera LED as output
    pinMode(leddoor, OUTPUT);     // Set regular door LED as output
    //-----------Turn off all leds-----------//
    digitalWrite(ledcam, LOW); // Turn cam LED OFF
    digitalWrite(ledcommun, LOW); // Turn communication LED OFF
    digitalWrite(ledcall, LOW); // Turn call LED OFF
    digitalWrite(leddoor, LOW); // Turn communication LED OFF
}

void init_call_button(void)
 {
   pinMode(callbutton, INPUT);      // Set call button as INPUT
   pullUpDnControl(callbutton, PUD_DOWN); // Enable pull-down resistor on button
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
  /*if (fcntl(zigbee_fd, F_SETFL, O_NONBLOCK) < 0) //Masqued Now
	{
	 perror("Unable set to NONBLOCK mode");
	 return 0;
	}*/ 

 return 1;
}

//send data to XBee 
int send_uart_data(char* pdata, int size)
{
 // interrupt=false;
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
  //interrupt=true;
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
    if (n==6)
    printf("%i bytes read : %s\n", n, pdata);
  }
return 0;
}

void zigbee_handle (/*door_visio *d*/)
    {
        //recieve_uart_data(d->packet_from_zigbee,8); 
       // printf("The value of d->packet_from_zigbee is:%s\n",door.packet_from_zigbee);            
        if (!strcmp(door.packet_from_zigbee,"DOOK"))// "DOOK" Porte Ouverte               
         {  
           printf("OKOKOK\n");           
           //active_led(&led_door);//Activate the open door led
           digitalWrite(leddoor, HIGH); // Turn door LED ON
           porte_ouverte();//Changing the door variable in database
           system("aplay -q /home/pi/Porte_Ouverte.wav");
         }

       if (!strcmp(door.packet_from_zigbee,"DOCL"))//DOCL Porte Fermee
        {  
         // stop_led(&led_door);//Desactivate the open door led
            digitalWrite(leddoor, LOW); // Turn door LED OFF 
          porte_fermee();//Changing the  door variable in database
          system("aplay -q /home/pi/Porte_Fermee.wav");                        
         }       
       if (!strcmp(door.packet_from_zigbee,"DOFO"))// "DOFO" Porte Forcee                                     
        {  
          porte_forcee();//Changing the door variable in database
          system("aplay -q /home/pi/Porte_Forcee.wav");                       
         }           
  
    }


