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
int fdbutton;
//int fdled0;//Led camera
int fdled1;//Led Appel
int fdled2;//Led communication
int fdled3;//Led porte
const char *fn;
char buf_Poll[4];

#define ERREXIT(str) {printf("err %s, %s\n", str, strerror(errno)); return -1;}

void Active_LED_Call(void);
void Stop_LED_Call(void);
void Active_LED_Communication(void);
void Stop_LED_Communication(void);
void Active_LED_Porte(void);
void Stop_LED_Porte(void);
//Button functions
void Init_Polling_Button(void);
void Polling_Button (void);
void Unexport_Polling_Button (void);

#endif

