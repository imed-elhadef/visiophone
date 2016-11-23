
/* $Id: pjsua_app_legacy.c 5065 2015-04-13 12:14:02Z nanang $ */
/*
 * Copyright (C) 2008-2011 Teluu Inc. (http://www.teluu.com)
 * Copyright (C) 2003-2008 Benny Prijono <benny@prijono.org>
* Copyright (C) 2016 Imed Elhadef <imed.elhadef@arcangel.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <pjsua-lib/pjsua.h>
#include "pjsua_app_common.h"
#include "visiophone.h" //Imed

//------------Imed Variables--------------//
extern Type_call call;
int index_client=0;
extern int client_number;
extern int press;
extern bool config_visiophone;
extern bool open_door;
extern bool rtsp_pi;
//-------------------Mysql Data---------------//
extern const char *server;
extern const char *user;
extern const char *password; /* set me first */
extern const char *database; 
    
//----------------------------------//
//-------------------NFC Data---------------//
extern bool verbose;
extern const uint8_t uiPollNr;//20;
extern const uint8_t uiPeriod;////2;
extern const nfc_modulation nmModulations[2];
extern const size_t szModulations;//2;
extern nfc_target nt;
extern int result ;
//----------------ZigBee Data-----------------//
extern char buffer_send[2];
//----------------Divers-----------------------//
extern t_call_status call_status;


#define THIS_FILE	"pjsua_app_legacy.c"


/* An attempt to avoid stdout buffering for python tests:
 * - call 'fflush(stdout)' after each call to 'printf()/puts()'
 * - apply 'setbuf(stdout, 0)', but it is not guaranteed by the standard:
 *   http://stackoverflow.com/questions/1716296
 */
#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) || \
    (defined (_MSC_VER) && _MSC_VER >= 1400)
/* Variadic macro is introduced in C99; MSVC supports it in since 2005. */
#  define printf(...) {printf(__VA_ARGS__);fflush(stdout);}
#  define puts(s) {puts(s);fflush(stdout);}
#endif

static pj_bool_t	cmd_echo;

//-------------Imed----------------//
  char* get_current_time(void)
  {
  static struct tm * timeinfo;//------------------------------> A voir le resultat
  time_t rawtime= time(0); // Get the system time
  time ( &rawtime );
  timeinfo = localtime ( &rawtime );
  return asctime (timeinfo);
  //printf ( "Current local time and date: %s", asctime (timeinfo) );
  }

/*
 * Print buddy list.
 */
static void print_buddy_list()
{
    pjsua_buddy_id ids[64];
    int i;
    unsigned count = PJ_ARRAY_SIZE(ids);

    puts("Buddy list:");

   pjsua_enum_buddies(ids, &count);
   
    if (count == 0) {
	puts(" -none-");
    } else {
	for (i=0; i<(int)count; ++i) {
	    pjsua_buddy_info info;

	    if (pjsua_buddy_get_info(ids[i], &info) != PJ_SUCCESS)
		continue;

	    printf(" [%2d] <%.*s>  %.*s\n",
		    ids[i]+1,
		    (int)info.status_text.slen,
		    info.status_text.ptr,
		    (int)info.uri.slen,
		    info.uri.ptr);
	}
    }
    puts("");
}

/*
 * Input URL.
 */
static void ui_input_url(const char *title, char *buf, pj_size_t len,
			 input_result *result)
{
    result->nb_result = PJSUA_APP_NO_NB;
    result->uri_result = NULL;

    print_buddy_list();
     
     printf("%s: ", title);
    
     
    strcpy(buf,"sip:192.168.1.123");
   
    len = strlen(buf);

    /* Left trim */
    while (pj_isspace(*buf)) {
	++buf;
	--len;
    }

    /* Remove trailing newlines */
    while (len && (buf[len-1] == '\r' || buf[len-1] == '\n'))
	buf[--len] = '\0';

    if (len == 0 || buf[0]=='q')
	return;

    if (pj_isdigit(*buf) || *buf=='-') {

	unsigned i;

	if (*buf=='-')
	    i = 1;
	else
	    i = 0;

	for (; i<len; ++i) {
	    if (!pj_isdigit(buf[i])) {
		puts("Invalid input");
		return;
	    }
	}

	result->nb_result = my_atoi(buf);

	if (result->nb_result >= 0 &&
	    result->nb_result <= (int)pjsua_get_buddy_count())
	{
	    return;
	}
	if (result->nb_result == -1)
	    return;

	puts("Invalid input");
	result->nb_result = PJSUA_APP_NO_NB;
	return;

    } else {
	pj_status_t status;

	if ((status=pjsua_verify_url(buf)) != PJ_SUCCESS) {
	    pjsua_perror(THIS_FILE, "Invalid URL", status);
	    return;
	}

	result->uri_result = buf;
    }
}

/** UI Command **/
static void ui_make_new_call()
{
    char buf[128];
    pjsua_msg_data msg_data;
    input_result result;
    pj_str_t tmp;

    printf("(You currently have %d calls)\n", pjsua_call_get_count());

    ui_input_url("Make call", buf, sizeof(buf), &result);
    if (result.nb_result != PJSUA_APP_NO_NB) {

	if (result.nb_result == -1 || result.nb_result == 0) {
	    puts("You can't do that with make call!");
	    return;
	} else {
	    pjsua_buddy_info binfo;
	    pjsua_buddy_get_info(result.nb_result-1, &binfo);
	    tmp.ptr = buf;
	    pj_strncpy(&tmp, &binfo.uri, sizeof(buf));
	}

    } else if (result.uri_result) {
	tmp = pj_str(result.uri_result);
    } else {
	tmp.slen = 0;
    }

    pjsua_msg_data_init(&msg_data);
    TEST_MULTIPART(&msg_data);
    pjsua_call_make_call(current_acc, &tmp, &call_opt, NULL,
			 &msg_data, &current_call);
}


/*
 * Main "user interface" loop.
 */
void legacy_main()
{
 int row_nbr =0;
  //-----------Change permission for serial driver------//
  system("sudo chmod 666 /dev/ttyAMA0");
  //-----------Destroy RTSP Server process if running---------------//
  system("sudo pkill mjpg_streamer");
 //---------------Init Button------------//        
  Unexport_Polling_Button();
  sleep(1);//You should add it for RPI
  Init_Polling_Button();
//----------------Init XBee Module------------------------//
 if (init_uart_port()== 0)
	{
        perror("Unable to initilize UART XBee");
	}
//-------------------MySQL Read Data Base-------------------//
/* read_visio_account();
 if (read_from_data_base() == 0)
	{
        perror("Unable to read from data base");
	}*/
//-------------------NFC Start-------------------//
 nfc_start();
//------------------------------------------------------//
    
    for (;;) 
    {
     Polling_Button();
     polling_normal_nfc();
     polling_config_value();//Poll les différents variables de config (config_visiophone,open_door et rtsp_pi)

     while (config_visiophone) //Check the config mode variable
      {
       config_visiophone=FALSE; //--> Reboot will make it FALSE
       polling_config_nfc();     
       }

      while (open_door) //Check the open door variable
      {
       open_door=FALSE;           
       //Ecriture dans la base de données 
      char *Querry = (char*) malloc(OFFSET_QUERRY_PREFIX);   
      sprintf(Querry, "UPDATE %s_parametre_visio SET ouvrir_porte_visio = '0'",prefix);
      if (mysql_query(conn, Querry))
     {
       fprintf(stderr, "%s\n", mysql_error(conn));
      } 
       //---------------Fin Ecriture-----------------//
       printf("Opening the door!!!\n");
       strcpy(buffer_send,"PO");// "PO" Ouverture de la porte (A voir la trame par la suite)
       send_uart_data(buffer_send,sizeof(buffer_send));
       free(Querry);//You have to free the allocated memory 
       Querry=0;    
       }
     
      if (rtsp_pi) //Check the mjpg rpi server variable
       {
       printf("Activating the mjpg_streamer!!!\n");
       system("/etc/init.d/mjpg-streamer.sh");
       }
     else
      system("sudo pkill mjpg_streamer");//Destroy rpi rtsp flux
 
     
     switch(call_status)
     {
        case end_call:
        call_status=idle;
        Stop_LED_Camera();//Closing LEDs camera
        Stop_LED_Communication(); //Close communication LED
        sleep(3); 
        system("aplay -q /home/pi/Call_end.wav");
        break;

        case time_out:
        call_status=idle;
        Stop_LED_Camera();//Closing LEDs camera
        Stop_LED_Communication(); //Close communication LED 
        sleep(3);
        system("aplay -q /home/pi/No_response.wav");
        break;

        case busy:
        call_status=idle;
        Stop_LED_Camera();//Closing LEDs camera
        Stop_LED_Communication(); //Close communication LED */

        case reject:
        call_status=idle;
        Stop_LED_Camera();//Closing LEDs camera
        Stop_LED_Communication(); //Close communication LED 
        sleep(3);
        system("aplay -q /home/pi/Call_reject.wav");
         
        default:
        printf("Nothing to do!!!\n");

      }
   
     while ((buf_Poll[0]==48) && (!press))

       { 
       printf("Test Button\n");
       //system("aplay -q /home/pi/Appel_en_cours.wav");
       ui_make_new_call();
       press=1;
       buf_Poll[0]=49;
       }
     
   /*  while ((buf_Poll[0]==48)&&(!press))
       {
        
        press=1;
  
        //Enregistrement des appels dans la base de données
         char *Querry1 = (char*) malloc(OFFSET_QUERRY_APPEL);
         char *Querry2 = (char*) malloc(OFFSET_QUERRY_PREFIX);      
         sprintf(Querry1, "INSERT INTO %s_appels_visio (id_appels_visio,time_appels_visio,id_user_visio) VALUES('','%s','1')",prefix,get_current_time()); 
        mysql_query(conn, Querry1);
        sprintf(Querry2, "SELECT * FROM %s_appels_visio",prefix);
        mysql_query(conn, Querry2);
        res = mysql_store_result(conn);
        row_nbr = mysql_num_rows(res);

        if (row_nbr == 101)//Maximum d'historique 100
         {
          sprintf(Querry2, "TRUNCATE TABLE %s_appels_visio",prefix);
          //mysql_query(conn,"TRUNCATE TABLE appels_visio");   
          mysql_query(conn, Querry2);
         }
        
        free(Querry1);//Free Allocated Memory
        free(Querry2);
        Querry1=0;
        Querry2=0;
       //---------------Fin Enregistrement-----------------//

        system("aplay -q /usr/bin/Appel_en_cours.wav");
        Active_LED_Call();
        if (call==Unicall)
        {
        printf("You are in Unicall module!!!\n");
        index_client=0;
        ui_make_new_call();
        }
        if (call==Multicall)
        {
        printf("You are in Multicall module!!!\n");
        for(index_client=0;index_client<client_number;index_client++)
        ui_make_new_call();
        usleep(200);
         } 
       buf_Poll[0]=49;
       }*/
   	
    }//End for loop

on_exit:
    ;
}
