/****************************************************************************/
 //   copyright            : (C) by 2016 Imed Elhadef
   
  
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
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>//For struct itimerval
#include <sys/signal.h>// for signal Interruption
#include <errno.h>
#include <sys/poll.h> //For polling File

typedef struct _led_visio 
{
 int fd;
 char pin_nbr[2];
} led_visio;


struct pollfd xfds[1];
int rc;//Poll
const char *fn;
char buf_Poll[4];

extern int fdbutton;//File descriptor of call button
extern int press;
extern led_visio led_cam;
#define ERREXIT(str) {printf("err %s, %s\n", str, strerror(errno)); return -1;}

//Leds functions
void active_led (led_visio *led);
void stop_led(led_visio *led);

//Button functions
void Init_Polling_Button(void);
void Polling_Button (void);
void Unexport_Polling_Button (void);

#endif

