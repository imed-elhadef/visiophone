
/* $Id: pjsua_app_legacy.c 5065 2015-04-13 12:14:02Z nanang $ */
/*
 * Copyright (C) 2014-2016 Imed Elhadef "Arcangel Technologies" <imed.elhadef@arcangel.fr>
 * Copyright (C) 2008-2011 Teluu Inc. (http://www.teluu.com)
 * Copyright (C) 2003-2008 Benny Prijono <benny@prijono.org>
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
#include "visiophone.h"
#include "database.h"
#include "MCP9808.h" 
//------------Imed Variables--------------//
//--------------mcp9808 Temp Sensor Data--------//
static const char* I2CDEV = "/dev/i2c-1"; //i2c-1 pour Raspberry
struct mcp9808 temp_sensor;
//------------ZigBee signal handler---------//
void signal_handler_IO (int status);//definition of signal handler 
struct sigaction saioUART; // definition of signal action 
//----------------Divers-----------------------//
static int index_client=0;
t_call_type call_history;
database_visio data_visio = {"",0,0,None,NULL};


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
    strcpy(buf,data_visio.sip_client_address[index_client]); 
    //strcpy(buf,"sip:192.168.1.123");
   
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
  //int fdWatchdog;         // File handler for watchdog
  //const char *WATCHDOGDEV = "/dev/watchdog0";// Watchdog default device file  
  float temperature = 0;//Ambient temperature 
  const char* gpio_button="16";//Raspberry Pi pin 16 for call button --> Active-low button - Broadcom pin 16, P1 pin 36
  //-----------Change permission for serial driver------//
  system("sudo chmod 666 /dev/ttyAMA0");
  //-----------Destroy RTSP Server process if running---------------//
  system("sudo pkill mjpg_streamer");
 //---------------Init Button------------//        
  Unexport_Polling_Button(gpio_button);
  sleep(1);//You should add it for RPI
  Init_Polling_Button(gpio_button);
//------Interrupt handler configuration-----//
        saioUART.sa_handler = signal_handler_IO;
        saioUART.sa_flags = 0;
        //saioUART.sa_mask = 0; //Masquer
        //sigemptyset(&saioUART.sa_mask);
        saioUART.sa_restorer = NULL; 
        sigaction(SIGIO,&saioUART,NULL);
//----------------Init XBee Module------------------------//
 if (init_uart_port()== 0)
	{
        perror("Unable to initilize UART XBee");
	}
//---------------Init mcp9808 Temp Sensor-------------------//
  while (!mcp9808_open(I2CDEV, MCP9808_ADR, &temp_sensor))
  printf ("Device checked with sucess!!!\n");
//-------------------MySQL Read Data Base-------------------//
 if(read_from_database(&data_visio))
  printf ("Database read with sucess!!!\n");
//-------------------NFC Start-------------------//
 nfc_start();
//------------------------------------------------------//

    for (;;) 
    { 
     Polling_Button();
     polling_normal_nfc();
     polling_config_value();//Poll les différents variables de config (config_visiophone,open_door et rtsp_pi, temperature)

     while (config_visiophone) //Check the config mode variable
      {
       config_visiophone=false; //--> Reboot will make it false
       polling_config_nfc();     
       }
      
      read_door_status(&door);
      read_mjpg_streamer_status(rtsp_pi);
      
      
    /*
      temperature = mcp9808_read_temperature(&temp_sensor);//Read the ambient temperature from mcp9880
      printf("temperature in celsius: %0.2f\n", temperature);
      write_temperature_to_database(temperature);  //Write to data base
    */
     
     switch(call_status)
     {
        case end_call:
        call_status=idle;
        call_history = received;
        stop_led(&led_cam);//Closing LEDs camera
        stop_led(&led_communication); //Close communication LED
        write_call_type_to_database(call_history);
        sleep(3); 
        system("aplay -q /home/pi/Call_end.wav");
        break;

        case time_out:
        call_status=idle;
        call_history=missed; //Appel en absence
        stop_led(&led_cam);//Closing LEDs camera
        stop_led(&led_communication); //Close communication LED
        write_call_type_to_database(call_history); 
        sleep(3);
        system("aplay -q /home/pi/No_response.wav");
        break;

        case busy:
        call_status=idle;
        call_history=missed; //Appel en absence
        stop_led(&led_cam);//Closing LEDs camera
        stop_led(&led_communication); //Close communication LED
        write_call_type_to_database(call_history);

        case reject:
        call_status=idle;
        call_history=missed; //Appel en absence
        stop_led(&led_cam);//Closing LEDs camera
        stop_led(&led_communication); //Close communication LED
        write_call_type_to_database(call_history); 
        sleep(3);
        system("aplay -q /home/pi/Call_reject.wav");
         
        //default:
        printf("Nothing to do!!!\n");
      }
   
     while ((buf_Poll[0]==48) && (!press))

      {
        press=1; 
        printf("Test Button\n");
        //system("aplay -q /home/pi/Appel_en_cours.wav");
        save_call_history_to_database();//Write to data base
        active_led(&led_call);

         if (data_visio.call_direction==Unicall)
           {
            printf("You are in Unicall module!!!\n");
            index_client=0;
            ui_make_new_call();
            }
          if (data_visio.call_direction==Multicall)
            {
             printf("You are in Multicall module!!!\n");
             for(index_client=0;index_client<data_visio.client_number;index_client++)
             ui_make_new_call();
             usleep(200);
             }
        buf_Poll[0]=49;
       }
   	
    }//End for loop

on_exit:
    ;
}

void signal_handler_IO (int status)
 {   
   //printf("Received data from XBee\n");    
   recieve_uart_data(door.packet_from_zigbee,8);
   //zigbee_handle (&door);
   zigbee_handle();

 }

