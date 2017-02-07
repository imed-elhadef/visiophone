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
#ifndef _VISIOPHONE_H_
#define _VISIOPHONE_H_

#include <string.h>
#include <stdio.h>
#include <stdlib.h>//System Call
#include <stdbool.h> //For bool type
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>//For struct itimerval
#include <sys/signal.h>// for signal Interruption
#include <errno.h>
#include <sys/poll.h> //For polling File
//----------UART-ZigBee--------------//
# include <termio.h>
#include <wiringPi.h> // Include WiringPi library!

typedef enum door_status
{
 closed,
 opened,
 forced
 } t_door_status;

typedef struct _serrure_data
{
  char* zigbee_data;
  t_door_status status;
  char* path;
  }
serrure_data;
//---Leds variables------------//
// Pin number declarations. We're using the Broadcom chip pin numbers.
extern const int ledcall; // Regular LED - Broadcom pin 26
extern const int ledcommun; // Regular LED - Broadcom pin 12
extern const int ledcam; // Regular LED - Broadcom pin 6
extern const int leddoor; // Regular LED - Broadcom pin 5
extern const int callbutton; // Active-low Capacitive button - Broadcom pin 16, P1 pin 36
//--------------------------//
#define ERREXIT(str) {printf("err %s, %s\n", str, strerror(errno)); return -1;}
int init_uart_port();
int send_uart_data(char* pdata, int size);//Send data to XBee
int receive_uart_data(char* pdata, int size);//receive data from XBee 
#endif

