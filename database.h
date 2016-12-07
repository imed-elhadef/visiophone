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
#ifndef _DATABASE_H_
#define _DATABASE_H_

#include <string.h>
#include <stdio.h>
#include <stdlib.h>//System Call
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>//For struct itimerval
#include <sys/signal.h>// for signal Interruption
#include <errno.h>
#include <mysql/mysql.h> //Mysql
//*********NFC****************//
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H
#include <err.h>
#include <inttypes.h>
#include <signal.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <nfc/nfc.h>
#include <nfc/nfc-types.h>
#include "nfc-utils.h"
//********ZigBee***********//
# include <termio.h>
//-----------MySQL--------------//
//#define OFFSET_QUERRY 64
#define OFFSET_QUERRY_APPEL 168
#define OFFSET_QUERRY_RFID 104
#define OFFSET_QUERRY_RECEPTEUR 84
#define OFFSET_QUERRY_PREFIX 60
#define SIZE_STRING_NFC 15
#define NFC_NUMBER   124
#define SIZE_CLIENT_NAME 24
#define SIZE_CLIENT_ADDRESS 128
#define CLIENT_NUMBER   8

typedef enum call_status
{
 reject,
 end_call,
 time_out,
 busy,
 idle
} t_call_status;

typedef enum call_direction 
{
 Unicall,
 Multicall,
 None
} t_call_direction;

typedef enum call_type
{
 missed,
 sent,
 received,
 none
 } t_call_type;

typedef struct _database_visio
 {
 // char prefix[5];//Prefix du compte admin visio
  int access_mode;
  int client_number; //Le nombre des clients à appeler
  t_call_direction call_direction;//enum type
  } database_visio;

typedef struct _door_visio
 {
  bool door_open;
  char packet_to_zigbee[5];
  char packet_from_zigbee[5];
  } door_visio;

typedef struct _mysql_config
  {
   //const char *server = "localhost";
   //const char *user = "root";
   //const char *password = "arcangel";         /* set me first */
   //const char *database = "visiophone";
   const char *server;
   const char *user;
   const char *password;         /* set me first */
   const char *database;
  } mysql_config;     

   
MYSQL *conn;
MYSQL_RES *res;
MYSQL_ROW row;
//-----------NFC---------------//
#define OFFSET 85
nfc_context *context;
//Base de Donnees NFC
 char NFC[NFC_NUMBER][SIZE_STRING_NFC]; //Les IDs des badges NFC
 char UID[15];
//-----------------------------------//
//char sip_client_address[128];
char sip_client_name[CLIENT_NUMBER][SIZE_CLIENT_NAME];//Le nom des équipements à appeler
char sip_client_address[CLIENT_NUMBER][SIZE_CLIENT_ADDRESS];//L'addresse complète des équipements à appeler
int access_mode;// Access Mode: 0 RFID / 1 Bouton capactif / 2 RFID & Bouton capactif
char ip_adress[16];
char prefix[5];//Prefix du compte admin visio

extern t_call_status call_status;
extern door_visio door;
extern bool config_visiophone;
extern short int rtsp_pi;

int config_nfc_target(const nfc_target *pnt, bool verbose);
//Mysql Functions
int read_from_database(database_visio *data_visio);
void polling_config_value(void);
//Write infos to data base
void write_temperature_to_database(float t);
void write_door_status_to_database(void);
void write_mjpg_status_to_database(void);
void write_call_type_to_database(t_call_type history);
void save_call_history_to_database(void);
void read_mjpg_streamer_status (int mjpg_status);
void read_door_status(door_visio *d);

#endif

