
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
#include "mcp9808.h" 
#include <linux/watchdog.h>
//------------Imed Variables--------------//
static int byte_nbr=0;
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
//-----------Watchdog-----------//
int fd_watch=-1;
char* watch_dev="/dev/watchdog";

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

//--------------------Watchdog-------------//
int init_watchdog(void)
  {
    int timeout = 7;
    if ((fd_watch = open(watch_dev, O_WRONLY)) <=0)
        {
                fprintf(stderr, "Error opening watchdog! %s \n", strerror(errno));
                return 0;
        }
       ioctl(fd_watch, WDIOC_SETTIMEOUT, &timeout);
       printf("The watchdog timeout is %d seconds.\n\n", timeout);
    return 1;   
   }
//----------------------------------------------------//

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
   int press=0;//Call button variable
  // int byte_nbr=0;
   unsigned char write_buf[] =
             {0x7E, 0x00, 0x19 , 0x11 , 0x01, 0x00, 0x17, 0x88, 0x01, 0x10, 0x57, 0x78, 0x7D, 0xFF, 0xFE, 0xE8, 0x0B, 0x00, 0x06, 0x01, 0x04, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x10, 0xE3};
  float temperature = 0;//Ambient temperature 
  //----------Change permission of watchdog file--------//
   system("sudo chmod 777 /dev/watchdog");
  //-----------Destroy RTSP Server process if running---------------//
  system("sudo pkill mjpg_streamer");
 //---------Initialize wiringPi-----------//
  wiringPiSetupGpio(); // Initialize wiringPi -- using Broadcom pin numbers
 //---------------Init Leds & Call Button------------//        
   init_visiophone_leds();
   init_call_button();
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
//-----------------Init watchdog---------//
  /* if (init_watchdog()== 0)
	{
        perror("Unable to initilize watchdog");
	}*/
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
     //write(fd_watch, "\0", 1);
     polling_normal_nfc();
     polling_config_value();//Poll les différents variables de config (config_visiophone,open_door et rtsp_pi, temperature)

     while (config_visiophone) //Check the config mode variable
      {
       config_visiophone=false; //--> Reboot will make it false
       polling_config_nfc();     
       }
      
      read_door_status(&door);
      read_mjpg_streamer_status(rtsp_pi);
       
    
      temperature = mcp9808_read_temperature(&temp_sensor);//Read the ambient temperature from mcp9880
      write_temperature_to_database(temperature);  //Write to data base

      /*byte_nbr=receive_uart_data(door.data_from_serrure,sizeof(door.data_from_serrure));
      if(byte_nbr==7)
      {
       byte_nbr=0;
       zigbee_handle();
      }*/
      while(byte_nbr==7)
      { 
       byte_nbr=0;
       zigbee_handle();
      }
    
     
     switch(call_status)
     {
        case end_call:
        press=0; 
        call_status=idle;
        call_history = received;
        digitalWrite(ledcam, LOW); // Turn cam LED OFF
        digitalWrite(ledcommun, LOW); // Turn communication LED OFF
        write_call_type_to_database(call_history);
        sleep(3); 
        system("aplay -q /home/pi/Call_end.wav");
        break;

        case time_out:
        press=0;
        call_status=idle;
        call_history=missed; //Appel en absence
        digitalWrite(ledcall, LOW); // Turn call LED OFF 
        write_call_type_to_database(call_history);
        //free_audio=false; 
        sleep(3);
        system("aplay -q /home/pi/No_response.wav");
        break;

        case busy:
        press=0;
        call_status=idle;
        call_history=missed; //Appel en absence
        digitalWrite(ledcall, LOW); // Turn call LED OFF 
        write_call_type_to_database(call_history);
        break;

        case reject:
        press=0;
        call_status=idle;
        call_history=missed; //Appel en absence
        digitalWrite(ledcall, LOW); // Turn call LED OFF 
        write_call_type_to_database(call_history); 
        sleep(3);
        system("aplay -q /home/pi/Call_reject.wav");
        break;

        case not_found:
        press=0;
        call_status=idle;
        digitalWrite(ledcall,LOW); // Turn call LED OFF 
        printf("Not Found!!!\n");
        //system("aplay -q /home/pi/Not_Found.wav");//Create a not_found received audio message
        break;

        case idle:
        press=0;
        break;        
 
        default:
        digitalWrite(ledcall, LOW); // Turn call LED OFF
        call_status=idle;
        //printf("Nothing to do!!!\n");
      }
   
    
    
          while ((digitalRead(callbutton)) && (!press)) // Button is released if this returns 1
         {
           press=1;              
           system("aplay -q /home/pi/Appel_en_cours.wav");
           digitalWrite(ledcall, HIGH); // Turn call LED ON
           save_call_history_to_database();//Write to data base

           if (data_visio.call_direction==Unicall)
            {
              //printf("You are in Unicall module!!!\n");
              index_client=0;
              ui_make_new_call();
             }
           if (data_visio.call_direction==Multicall)
            {
              //printf("You are in Multicall module!!!\n");
              for(index_client=0;index_client<data_visio.client_number;index_client++)
              ui_make_new_call();
              usleep(200);
             }
        }
        
   	
    }//End for loop

on_exit:
    ;
}

void signal_handler_IO (int status)
 {     
      byte_nbr=receive_uart_data(door.data_from_serrure,sizeof(door.data_from_serrure));
     /* if(byte_nbr==7)
      { 
       zigbee_handle();
      }*/
}
