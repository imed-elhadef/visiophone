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
#include <mysql/mysql.h> //Mysql
//*********NFC****************//
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H
#include <err.h>
#include <inttypes.h>
#include <signal.h>
//#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <nfc/nfc.h>
#include <nfc/nfc-types.h>
#include "nfc-utils.h"
//********ZigBee***********//
# include <termio.h>
//********HTTP**********//
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
//*********************//

#define TRUE 1
#define FALSE 0
//-----------MySQL--------------//
//#define OFFSET_QUERRY 64
#define OFFSET_QUERRY_APPEL 120
#define OFFSET_QUERRY_RFID 84
#define OFFSET_QUERRY_RECEPTEUR 84
#define OFFSET_QUERRY_PREFIX 60
#define SIZE_STRING_NFC 15
#define NFC_NUMBER   124
#define SIZE_CLIENT_ADDRESS 128
#define CLIENT_NUMBER   8

typedef enum call
{
 reject,
 end_call,
 time_out,
 busy,
 idle
} t_call_status;

typedef enum type_call 
{
 Unicall,
 Multicall,
 None
} Type_call;

MYSQL *conn;
MYSQL_RES *res;
MYSQL_ROW row;
//-----------NFC---------------//
#define OFFSET 85
nfc_context *context;
//Base de Donnees Temporaire RFID
//char* NFC[4] ;
 char NFC[NFC_NUMBER][SIZE_STRING_NFC]; 
 char UID[15];
//-----------------------------------//
char prefix[5];
//char sip_client_address[128];
char sip_client_id[CLIENT_NUMBER][SIZE_CLIENT_ADDRESS];
int access_mode;// Access Mode: 0 RFID / 1 Bouton capactif / 2 RFID & Bouton capactif
struct pollfd xfds[1];
int rc;//Poll
int fdbutton;
int fdled0;//Led camera
int fdled1;//Led Appel
int fdled2;//Led communication
int fdled3;//Led porte
const char *fn;
char buf_Poll[4];

#define ERREXIT(str) {printf("err %s, %s\n", str, strerror(errno)); return -1;}

//Button functions
void Init_Polling_Button(void);
void Polling_Button (void);
void Unexport_Polling_Button (void);
//NFC Functions
void nfc_start(void);
void polling_normal_nfc(void);
void polling_config_nfc(void);
void normal_nfc_target(const nfc_target *pnt, bool verbose);
int config_nfc_target(const nfc_target *pnt, bool verbose);
//Mysql Functions
int read_from_data_base(void);
void polling_config_value(void);

#endif

