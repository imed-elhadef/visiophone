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
//---Leds variables------------//
typedef struct _led_visio 
{
 int fd;
 char pin_nbr[2];
 char fn_led[34];
} led_visio;
extern led_visio led_call;
extern led_visio led_communication;
extern led_visio led_cam;
//--------------------------//
//---Call button variables------------//
struct pollfd xfds[1];
int rc;//Poll
char fn[34];
char buf_Poll[4];
extern int fdbutton;//File descriptor of the call button
extern int press;
//---------------------------------------//
extern bool interrupt;
#define ERREXIT(str) {printf("err %s, %s\n", str, strerror(errno)); return -1;}

//Leds functions
void active_led (led_visio *led);
void stop_led(led_visio *led);
//Button functions
void Init_Polling_Button(const char* pin_nbr);
void Polling_Button (void);
void Unexport_Polling_Button (const char* pin_nbr);
//ZigBee functions
int init_uart_port();
int send_uart_data(char* pdata, int size);//Send data to XBee
int recieve_uart_data(char* pdata, int size);//receive data from XBee 
#endif

