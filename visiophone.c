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
#include "visiophone.h"


Type_call call=None;
int press=0;
int client_number=0;//Nombre des destinataires à appeler
static int badge_number=0;//Nombre des badges dans la base de données
bool config_visiophone=FALSE;//L'interphone fonctionne par défaut en "Mode Normale" --> Mode Normale=0 Mode Config=1
bool open_door=FALSE;//Par défaut la porte est fermée
bool rtsp_pi=FALSE;//Par défaut le serveur mjpg_streamer de la camera Rpi2 ne fonctionne plus
const char *server = "localhost";
const char *user = "root";
const char *password = "arcangel";         /* set me first */
const char *database = "visiophone";     
//-------------------NFC Data---------------//
nfc_device *pnd = NULL;
bool verbose = false;
const uint8_t uiPollNr = 1;//20;
const uint8_t uiPeriod = 1;////2;
const nfc_modulation nmModulations[2] = {
    { .nmt = NMT_ISO14443A, .nbr = NBR_106 },
    { .nmt = NMT_ISO14443B, .nbr = NBR_106 },
    //{ .nmt = NMT_FELICA, .nbr = NBR_212 },
    //{ .nmt = NMT_FELICA, .nbr = NBR_424 },
   // { .nmt = NMT_JEWEL, .nbr = NBR_106 },
  };
const size_t szModulations = 1;//2;
nfc_target nt;
int result = 0;
//-------------------ZigBee-------------------//
int zigbee_fd=-1;
char buffer_send[2];
// Read up to 24 characters from the port if they are there
char buf[24];
const char* serial_port= "/dev/ttyAMA0";//Serial Port for raspberry
//------------------HTTP SEVER-------------//
/*#define REPLY_SIZE 1024
int socket_desc;
struct sockaddr_in httpserver;
char *message , server_reply[REPLY_SIZE];*/
//-----------------------------------------//

void Active_LED_Call(void)
{
 /* export */
  fn = "/sys/class/gpio/export";
  fdled1 = open(fn, O_WRONLY);
  if (fdled1 < 0)
  ERREXIT("open export")

 //Write our value of "26" to the file
  write(fdled1,"26",2);
  close(fdled1);  
  printf("...export file accessed, new pin now accessible\n");

  sleep(1); //---> You must add it for RPI 

 //SET DIRECTION
  //Open the LED's sysfs file in binary for reading and writing, store file pointer in fp
  fn ="/sys/class/gpio/gpio26/direction";
  fdled1 = open(fn,O_RDWR);

  if (fdled1 < 0) 
  ERREXIT("open direction")
  write(fdled1,"out",3);
  close(fdled1);
  printf("...direction set to output\n");
  
//Set Value
  fn="/sys/class/gpio/gpio26/value";
  fdled1 = open(fn,O_RDWR);
  if (fdled1 < 0) 
  ERREXIT("open value")//1:LED ON \ 0:LED OFF
  write(fdled1,"1",1);   
}

void Stop_LED_Call(void)
     {
    write(fdled1,"0",1);//Disable LED
    close(fdled1);
    fdled1=-1;
      }

void Active_LED_Communication(void)
{
 /* export */
  fn = "/sys/class/gpio/export";
  fdled2 = open(fn, O_WRONLY);
  if (fdled2 < 0)
  ERREXIT("open export")

 //Write our value of "12" to the file
  write(fdled2,"12",2);
  close(fdled2);  
  printf("...export file accessed, new pin now accessible\n");

  sleep(1); //---> You must add it for RPI

 //SET DIRECTION
  //Open the LED's sysfs file in binary for reading and writing, store file pointer in fp
  fn ="/sys/class/gpio/gpio12/direction";
  fdled2 = open(fn,O_RDWR);

  if (fdled2 < 0) 
  ERREXIT("open direction")
  write(fdled2,"out",3);
  close(fdled2);
  printf("...direction set to output\n");
  
//Set Value
  fn="/sys/class/gpio/gpio12/value";
  fdled2 = open(fn,O_RDWR);
  if (fdled2 < 0) 
  ERREXIT("open value")//1:LED ON \ 0:LED OFF
  write(fdled2,"1",1);   
}

void Stop_LED_Communication(void)
     {
    write(fdled2,"0",1);//Disable LED
    close(fdled2);
    fdled2=-1;
      }

void Active_LED_Porte(void)
{
 /* export */
  fn = "/sys/class/gpio/export";
  fdled3 = open(fn, O_WRONLY);
  if (fdled3 < 0)
  ERREXIT("open export")

 //Write our value of "5" to the file
  write(fdled3,"5",1);
  close(fdled3);  
  printf("...export file accessed, new pin now accessible\n");

  sleep(1); //---> You must add it for RPI

 //SET DIRECTION
  //Open the LED's sysfs file in binary for reading and writing, store file pointer in fp
  fn ="/sys/class/gpio/gpio5/direction";
  fdled3 = open(fn,O_RDWR);

  if (fdled3 < 0) 
  ERREXIT("open direction")
  write(fdled3,"out",3);
  close(fdled3);
  printf("...direction set to output\n");
  
//Set Value
  fn="/sys/class/gpio/gpio5/value";
  fdled3 = open(fn,O_RDWR);
  if (fdled3 < 0) 
  ERREXIT("open value")//1:LED ON \ 0:LED OFF
  write(fdled3,"1",1);   
}

void Stop_LED_Porte(void)
     {
    write(fdled3,"0",1);//Disable LED
    close(fdled3);
    fdled3=-1;
      }

void Init_Polling_Button(void) //Raspberry Pi pin 16 for call button
{
  /* export GPIO 16 */  
  fn = "/sys/class/gpio/export";
  fdbutton = open(fn, O_WRONLY); if(fdbutton < 0)  ERREXIT("open export")
  rc = write(fdbutton, "16", 3); if(rc != 3) ERREXIT("write export")//Active-low button - Broadcom pin 16, P1 pin 36
  close(fdbutton);

  sleep(1); //---> You must add it for RPI
  /* direction */
  
  fn = "/sys/class/gpio/gpio16/direction";//16
  fdbutton = open(fn, O_RDWR);  if(fdbutton < 0)  ERREXIT("open direction")
  rc = write(fdbutton, "in", 3);if(rc != 3) ERREXIT("write direction")
  close(fdbutton);
  
  /* edge */

  fn = "/sys/class/gpio/gpio16/edge";//16
  fdbutton = open(fn, O_RDWR);        if(fdbutton < 0)  ERREXIT("open edge")
  rc = write(fdbutton, "falling", 8); if(rc != 8) ERREXIT("write edge")
  rc = lseek(fdbutton, 0, SEEK_SET);  if(rc < 0)  ERREXIT("lseek edge")
  rc = read(fdbutton, buf_Poll, 10);       if(rc <= 0) ERREXIT("read edge")
  buf_Poll[10] = '\0';
  printf("read gpio16/edge:%s\n", buf_Poll);//16
  close(fdbutton);

  /* wait for interrupt - try it a few times */
  fn = "/sys/class/gpio/gpio16/value";//16
  fdbutton = open(fn, O_RDWR);  if(fdbutton < 0)  ERREXIT("open value")
  xfds[0].fd       = fdbutton;
  xfds[0].events   = POLLPRI | POLLERR;
  xfds[0].revents  = 0;

 }

void Polling_Button (void)
 {     
        //printf("Waiting for interrupts!!!\n");  
  	rc = poll(xfds, 1, 500); if(rc == -1) ERREXIT("poll value") //500 polling time
	rc = lseek(fdbutton, 0, SEEK_SET); if (rc < 0)  ERREXIT("lseek value")
  	rc = read(fdbutton, buf_Poll, 2); if (rc != 2) ERREXIT("read value")
  	buf_Poll[1] = '\0'; // Overwrite the newline character with terminator
    
       if (buf_Poll[0]==49)
        press=0;
}
 
 void Unexport_Polling_Button (void)
{
  /* unexport GPIO 16*/ 
  fn = "/sys/class/gpio/unexport";
  fdbutton = open(fn, O_WRONLY); if(fdbutton < 0)  ERREXIT("open unexport")
  rc = write(fdbutton, "16", 3); if(rc != 3) ERREXIT("write unexport")//16
  close(fdbutton);
 }

//******************NFC************************//
void nfc_start(void)
{
  nfc_init(&context);
  if (context == NULL) {
    ERR("Unable to init libnfc (malloc)");
    exit(EXIT_FAILURE);
  }

  pnd = nfc_open(context, NULL);

  if (pnd == NULL) {
    ERR("%s", "Unable to open NFC device.");
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  if (nfc_initiator_init(pnd) < 0) {
    nfc_perror(pnd, "nfc_initiator_init");
    nfc_close(pnd);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  printf("NFC reader: %s opened\n", nfc_device_get_name(pnd));
  printf("NFC device will poll during %ld ms (%u pollings of %lu ms for %" PRIdPTR " modulations)\n", (unsigned long) uiPollNr *      szModulations * uiPeriod * 150, uiPollNr, (unsigned long) uiPeriod * 150, szModulations);
}

void polling_normal_nfc(void)
{
result = nfc_initiator_poll_target(pnd, nmModulations, szModulations, uiPollNr, uiPeriod, &nt);
     if (result > 0) 
      {
       normal_nfc_target(&nt, verbose);
      }
 }

void polling_config_nfc()
{
 result = nfc_initiator_poll_target(pnd, nmModulations, szModulations, uiPollNr, uiPeriod, &nt);
       if (result > 0) 
       	config_nfc_target(&nt, verbose);
}

void normal_nfc_target(const nfc_target *pnt, bool verbose)
{
  int i;
  char *s;
  printf("Normal Read Done!\n");
  str_nfc_target(&s, pnt, verbose);
  memcpy(UID,s+OFFSET,15);
  system("aplay -q  /home/pi/Beep.wav");
      for (i=0; i<badge_number; i++)
          { 
           if (strncmp(NFC[i],UID,14) == 0)
             {
              printf("Test Good\n");
              //buffer_send [0] = 0x50;// 'P' Ouverture de la porte (A voir)
              strcpy(buffer_send,"PO");// "PO" Ouverture de la porte (A voir)
              send_uart_data(buffer_send,sizeof(buffer_send));
              break;
             } 
          }
  nfc_free(s);
}

int config_nfc_target(const nfc_target *pnt, bool verbose)
{
  int number=1;
  char *s;
  printf("Config Read Done!\n");
  str_nfc_target(&s, pnt, verbose);
  memcpy(UID,s+OFFSET,15);
  system("aplay -q  /home/pi/Beep.wav");

 //Ecriture dans la base de données   
   char *Querry = (char*) malloc(OFFSET_QUERRY_RFID);
   sprintf(Querry, "INSERT INTO %s_badge_rfid (id_badge_RFID,id_user_visio) VALUES('%s','1')",prefix,UID); 
   //sprintf(Querry, "INSERT INTO badge_RFID (id_badge_RFID, date_ajout_badge_RFID) VALUES('%s','%s')",UID,get_current_time()); 
   if (mysql_query(conn, Querry)) {
   fprintf(stderr, "%s\n", mysql_error(conn));
    } 
   free(Querry);//Free Allocated Memory
   nfc_free(s);
  
   return 0;
}
//******************************************//

//*****************Data Base*************//
/* Connect to database */

int read_visio_account (void) //Lecture de la compte admin de visiophone
{
 conn = mysql_init(NULL);
   if (!mysql_real_connect(conn, server,
         user, password, database, 0, NULL, 0)) {
      fprintf(stderr, "%s\n", mysql_error(conn));
      exit(1);
   }
   else
   printf ("Connected with success!!!\n");

  //Lecture dans la base de données des badges RFID
   if (mysql_query(conn, "SELECT * FROM comptes_visio WHERE role_comptes_visio ='administrateur'")) 
  {
      fprintf(stderr, "%s\n", mysql_error(conn));
      return 0;
  }

  res = mysql_store_result(conn); 
  row = mysql_fetch_row(res);
  strcpy(prefix,row[4]);
  printf("The prefix is:%s\n",prefix);
  //Disconnect from Data Base
   mysql_free_result(res);
   

 return 1;
}
int read_from_data_base(void)
{
   char *Querry1 = (char*)malloc(OFFSET_QUERRY_PREFIX); 
   char *Querry2 = (char*)malloc(OFFSET_QUERRY_RECEPTEUR);      
   //Lecture dans la base de données des badges RFID  
   sprintf(Querry1, "SELECT * FROM %s_badge_rfid",prefix); 
   if (mysql_query(conn, Querry1))
   {
      fprintf(stderr, "%s\n", mysql_error(conn));
      return 0;
  }
 
  res = mysql_store_result(conn);
   
  if (res == NULL) 
  {
      fprintf(stderr, "%s\n", mysql_error(conn));
      return 0;
  }
  while ((row = mysql_fetch_row(res))) 
  { 
     strcpy(NFC[badge_number],row[0]);//Save RFID IDs in NFC table
     printf("%s\n", NFC[badge_number]);
     badge_number++;
  }
  
//Lecture dans la base de données des paramètres Visiophone
   sprintf(Querry1, "SELECT * FROM %s_parametre_visio",prefix); 
   if (mysql_query(conn, Querry1))
   {
      fprintf(stderr, "%s\n", mysql_error(conn));
      return 0;
  }

  res = mysql_store_result(conn);
  if (res == NULL) 
  {
      fprintf(stderr, "%s\n", mysql_error(conn));
      return 0;
  }

   row = mysql_fetch_row(res);    
   access_mode = row[4];// Access Mode: 0 RFID / 1 Bouton capactif / 2 RFID & Bouton capactif
   printf("Acess mode is: %d\n", access_mode);
   
 //Lecture dans la base de données equipement recepteur   
   sprintf(Querry2, "SELECT * FROM %s_equipement_recepteur WHERE activation_equipement_recepteur='1'",prefix); 
   if (mysql_query(conn, Querry2))
   {
      fprintf(stderr, "%s\n", mysql_error(conn));
      return 0;
   }
  res = mysql_store_result(conn);
  if (res == NULL) 
  {
      fprintf(stderr, "%s\n", mysql_error(conn));
      return 0;
  }
  
   while ((row = mysql_fetch_row(res))) 
  { 
     memset(sip_client_id[client_number],0,sizeof(sip_client_id[client_number]));  
     strcpy(sip_client_id[client_number],row[5]);//Save les address des equipement à appeler in sip_client_id table
     printf("The client adress is: %s\n", sip_client_id[client_number]);
     client_number++;
     printf("The client numbers is:%d\n",client_number);
  }
   
  if (client_number<=0)
  call=None;
  else if (client_number==1)
  call=Unicall;
  else
  call=Multicall;
  
    
  free(Querry1); //Libérer la mémoire
  free(Querry2); //Libérer la mémoire
  //Disconnect from Data Base
   mysql_free_result(res);
   //mysql_close(conn);
   return 1;
}

void polling_config_value(void)
{
 //Lecture dans la base de données de la paramètre config
   //  mysql_query(conn, "SELECT * FROM parametre_visio"); //False --> Mode Normale   TRUE --> Mode config 
     char *Querry = (char*) malloc(OFFSET_QUERRY_PREFIX);    
     sprintf(Querry, "SELECT * FROM %s_parametre_visio",prefix); 
     mysql_query(conn, Querry);
     res = mysql_store_result(conn); 
     row = mysql_fetch_row(res);        
     printf("%d\n",atoi(row[3]));
     printf("%d\n",atoi(row[9]));
     printf("%d\n",atoi(row[10]));//Ajouter un nouveau champ pour la camera rtsp
     if(atoi(row[3]))
     config_visiophone=TRUE; 
     if(atoi(row[9]))
     open_door=TRUE;
     if(atoi(row[10]))
     rtsp_pi=TRUE;
     else
     rtsp_pi=FALSE;        
     free(Querry);   
   //Disconnect from Data Base
     mysql_free_result(res);
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
 

  // Turn off blocking for reads, use (fd, F_SETFL, FNDELAY) if you want that
  if (fcntl(zigbee_fd, F_SETFL, O_NONBLOCK) < 0)
	{
	 perror("Unable set to NONBLOCK mode");
	 return 0;
	}

 return 1;
}
//send XBee data 
int send_uart_data(char* pdata, int size)
{
// Write to the port
  int n = write(zigbee_fd,pdata,size);
  if (n < 0) {
    perror("Write failed - ");
    return 1;
  }
return 0;
}
//receive XBee data 
int recieve_uart_data(char* pdata, int size)
{
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
    printf("%i bytes read : %s\n", n, pdata);
  }
return 0;
}

//********************Etat Porte******************//

void porte_ouverte()
{
 //Ecriture dans la base de données   
       //if(mysql_query(conn, "UPDATE portier_visio SET etat_portier_visio = '1'")) 
     char *Querry = (char*) malloc(OFFSET_QUERRY_PREFIX);   
     sprintf(Querry, "UPDATE %s_parametre_visio SET etat_portier_visio = '1'",prefix); 
     if (mysql_query(conn, Querry))
      {
        fprintf(stderr, "%s\n", mysql_error(conn));
      } 
       //---------------Fin Ecriture-----------------//
     free(Querry);
}

void porte_fermee()
{
 //Ecriture dans la base de données   
       //if (mysql_query(conn, "UPDATE portier_visio SET etat_portier_visio = '0'")) 
     char *Querry = (char*) malloc(OFFSET_QUERRY_PREFIX);   
     sprintf(Querry, "UPDATE %s_parametre_visio SET etat_portier_visio = '0'",prefix); 
     if (mysql_query(conn, Querry))
     {
       fprintf(stderr, "%s\n", mysql_error(conn));
      } 

       //---------------Fin Ecriture-----------------//
     free(Querry);
}

void porte_forcee()
{

//Ecriture dans la base de données   
      // if (mysql_query(conn, "UPDATE portier_visio SET etat_portier_visio = '2'")) 
     char *Querry = (char*) malloc(OFFSET_QUERRY_PREFIX);   
     sprintf(Querry, "UPDATE %s_parametre_visio SET etat_portier_visio = '2'",prefix); 
     if (mysql_query(conn, Querry))
      {
       fprintf(stderr, "%s\n", mysql_error(conn));
      } 
 
       //---------------Fin Ecriture-----------------//
     free(Querry);
}
//********************HTTP SERVER*****************//

 /*int init_http_socket()
  {
      //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
         
    httpserver.sin_addr.s_addr = inet_addr("127.0.0.1");//localhost
    httpserver.sin_family = AF_INET;
    httpserver.sin_port = htons( 80 );
 
    //Connect to remote http server
     if (connect(socket_desc,(struct sockaddr *)&httpserver,sizeof(httpserver)) < 0)
     {
        puts("connect error");
        return 1;
     }
     
     puts("Connected\n");
  
      if (fcntl(socket_desc, F_SETFL, O_NDELAY) < 0) 
        {
		perror("Can't set socket to non-blocking");
		exit(0);
	}
         
     return 0;     

   }

  void send_http_socket ()
   {
     //Send some data
     // message = "/videophone/index.php?param1=9";
     //message = "GET /videophone/index.php?param1=5 HTTP/1.0\r\n\r\n";
     // message = "GET /videophone/index.php?param1=2 HTTP/1.0\r\n\r\n"; --> Marche
     //message = "GET /login-user.php HTTP/1.0\r\n\r\n";
    
     message = " PUT /videophone/index.php?param1=1 \r\n\r\n";//---> A changer la page

     if( send(socket_desc , message , strlen(message) , 0) < 0)
      {
        puts("Send failed");
        return 1;
      }

     puts("Data Sent\n");
    }
 

  void read_http_socket ()
   {
    //Send some data

     // message = "/videophone/index.php?param1=9";
     //message = "GET /videophone/index.php?param1=5 HTTP/1.0\r\n\r\n";
    // message = "GET /videophone/index.php?param1=2 HTTP/1.0\r\n\r\n"; --> Marche
     message = "GET /login-user.php HTTP/1.0\r\n\r\n";
    
     //message = " PUT /videophone/index.php?param1=1 \r\n\r\n";

    if( send(socket_desc , message , strlen(message) , 0) < 0)
    {
        puts("Send failed");
       // return 1;
    }

   // puts("Data Sent\n");
     
    //Receive a reply from the server
    if( recv(socket_desc, server_reply , sizeof(server_reply) , 0) < 0)
    {
        puts("recv failed");
    }
    puts("Reply received\n");
    //puts(server_reply);
    //puts(server_reply + OFFSET_HTTP + 4000);
    //Copy data here 
    }
    */
