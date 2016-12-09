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
#include "database.h"
mysql_config mysql_conf ={.server="localhost",.user="root",.password="arcangel",.database="visiophone"};
door_visio door = {false,"OPEN",""};//Par défaut la porte est fermée 
static int badge_number=0;//Nombre des badges dans la base de données
bool config_visiophone=false;//L'interphone fonctionne par défaut en "Mode Normale" --> Mode Normale=0 Mode Config=1
short int rtsp_pi=0;//Par défaut le serveur mjpg_streamer de la camera Rpi2 ne fonctionne plus
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
char buffer_send[5];
// Read up to 24 characters from the port if they are there
//char buf[24];
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
              //strcpy(buffer_send,"OPEN");// "OPEN" Ouverture de la porte (A voir)
              strcpy(buffer_send,door.packet_to_zigbee);// "OPEN" Ouverture de la porte (A voir)
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
  str_nfc_target(&s, pnt, verbose);
  memcpy(UID,s+OFFSET,15);
  system("aplay -q  /home/pi/Beep.wav");
 //Ecriture dans la base de données   
   char *Querry = (char*) malloc(OFFSET_QUERRY_RFID);
   sprintf(Querry, "INSERT INTO %s_badge_rfid (id_badge_RFID,activation_badge_RFID) VALUES('%s','1')",prefix,UID); 
   //sprintf(Querry, "INSERT INTO badge_RFID (id_badge_RFID, date_ajout_badge_RFID) VALUES('%s','%s')",UID,get_current_time()); 
   if (mysql_query(conn, Querry)) {
   fprintf(stderr, "%s\n", mysql_error(conn));
    } 
   free(Querry);//Free Allocated Memory
   Querry=NULL;
   nfc_free(s);

   return 0;
}
//*****************Data Base*************//
int read_from_database(database_visio *data_visio)
{    
   char *Querry1 = (char*)malloc(OFFSET_QUERRY_PREFIX); 
   char *Querry2 = (char*)malloc(OFFSET_QUERRY_RECEPTEUR);      
   //Connect to database
   conn = mysql_init(NULL);
   if (!mysql_real_connect(conn, mysql_conf.server,
         mysql_conf.user, mysql_conf.password, mysql_conf.database, 0, NULL, 0)) {
      fprintf(stderr, "%s\n", mysql_error(conn));
      exit(1);
   }
   else
   printf ("Connected with success!!!\n");

  //Lecture de table compte_visio pour enregistrer le prefix du compte de type administrateur
   if (mysql_query(conn, "SELECT * FROM comptes_visio WHERE role_comptes_visio ='administrateur'")) 
  {
      fprintf(stderr, "%s\n", mysql_error(conn));
      return 0;      
  }

  res = mysql_store_result(conn); 
  row = mysql_fetch_row(res);
  strcpy(prefix,row[4]);
  printf("The prefix is:%s\n",prefix);
 

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
   data_visio->access_mode = atoi(row[3]);//column mode_acces_visiophone --> Access Mode: 0 RFID / 1 Bouton capactif / 2 RFID & Bouton capactif
   strcpy(ip_adress,row[1]);// IP adress de visiophone
   printf("Acess mode is: %d\n", data_visio->access_mode);
   printf("IP adress: %s\n", ip_adress);

   //Lecture dans la base de données des badges RFID  
   sprintf(Querry2, "SELECT * FROM %s_badge_rfid WHERE activation_badge_RFID = '1'",prefix); 
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
     strcpy(NFC[badge_number],row[0]);//Save RFID IDs in NFC table --> Maximum badges to save is 124
     printf("%s\n", NFC[badge_number]);
     badge_number++;
  }
  
   
 //Lecture dans la base de données les equipements recepteurs   
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
     strcpy(sip_client_name[data_visio->client_number],row[1]);//Sauvegarder les noms des addresses des equipement à appeler--> Max 8 clients
     printf("The client name is: %s\n", sip_client_name[data_visio->client_number]);
     snprintf(data_visio->sip_client_address[data_visio->client_number],sizeof(data_visio->sip_client_address[data_visio->client_number]),"sip:%s@%s",sip_client_name[data_visio->client_number],ip_adress);//Concatiner les le nom avec l'adresse ip pour avoir l'adresse d'appel
     printf("The client address is: %s\n", data_visio->sip_client_address[data_visio->client_number]);
     data_visio->client_number++;
     printf("The client numbers is:%d\n",data_visio->client_number);
  }
   
  if (data_visio->client_number<=0)
  data_visio->call_direction=None;
  else if (data_visio->client_number==1)
  data_visio->call_direction=Unicall;
  else
  data_visio->call_direction=Multicall;
  
  free(Querry1); //Libérer la mémoire
  free(Querry2); //Libérer la mémoire
  Querry1 = NULL;
  Querry2 = NULL;
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
     printf("%d\n",atoi(row[2]));//Column mode_conf_visiophone --> Pour sélectionner mode config et mode normale
     printf("%d\n",atoi(row[6]));//Column ouvrir_porte_visio --> Detecte si la porte est ouverte ou fermée
     printf("%d\n",atoi(row[7]));//Ajouter un nouveau champ pour la camera rtsp nommé "mjpg_streamer"
     if(atoi(row[2]))
     config_visiophone=true; 
     if(atoi(row[6]))
     door.door_open/*open_door*/=true;
     printf("pjpg_streamer value:%d\n",atoi(row[8]));
     if(atoi(row[7]) == 2)
     rtsp_pi=2;
     else if (atoi(row[7]) == 1)
     rtsp_pi=1;
     else      
     rtsp_pi=0;  
     free(Querry);   
   //Disconnect from Data Base
     mysql_free_result(res);
  }

//********************Acess Door functions******************//
 void read_door_status(door_visio *d)
 {
   static bool index=true;
   if ((d->door_open) && (index)) //Check the open door variable
      {
       //door_var=FALSE;           
       //write_door_status_to_database();//Write to data base
       index=false;  
       printf("Opening the door!!!\n");
       strcpy(buffer_send,d->packet_to_zigbee);// "OPEN" Ouverture de la porte (A voir la trame par la suite)
       send_uart_data(buffer_send,sizeof(buffer_send));
       }
   else if ((d->door_open) && (!index))
    printf("The door is already opened!!!\n");
     
  }

void porte_fermee()
{
     //Ecriture dans la base de données    
     char *Querry = (char*) malloc(OFFSET_QUERRY_PREFIX);   
     sprintf(Querry, "UPDATE %s_porte_visio SET etat_porte_visio = '0'",prefix); //Write 0 for closed door
     if (mysql_query(conn, Querry))
     {
       fprintf(stderr, "%s\n", mysql_error(conn));
      } 

       //---------------Fin Ecriture-----------------//
     free(Querry);
     Querry=NULL;
}

void porte_ouverte()
{
     //Ecriture dans la base de données   
     char *Querry = (char*) malloc(OFFSET_QUERRY_PREFIX);   
     sprintf(Querry, "UPDATE %s_porte_visio SET etat_porte_visio = '1'",prefix); //Write 1 for opened door
     if (mysql_query(conn, Querry))
      {
        fprintf(stderr, "%s\n", mysql_error(conn));
      } 
       //---------------Fin Ecriture-----------------//
     free(Querry);
     Querry=NULL;
}

 void porte_forcee()
  {
    //Ecriture dans la base de données   
     char *Querry = (char*) malloc(OFFSET_QUERRY_PREFIX);   
     sprintf(Querry, "UPDATE %s_porte_visio SET etat_porte_visio = '2'",prefix); //Write 2 for forced door 
     if (mysql_query(conn, Querry))
      {
       fprintf(stderr, "%s\n", mysql_error(conn));
      } 
       //---------------Fin Ecriture-----------------//
     free(Querry);
     Querry=NULL;
  }
//********************Mjpg Streamer*********************//
void read_mjpg_streamer_status (int mjpg_status)
 {
  if (mjpg_status==2) //Check the mjpg rpi server variable
       {
       printf("Activating the mjpg_streamer!!!\n");
       system("/etc/init.d/mjpg-streamer.sh");
       }
     else if (mjpg_status==1)
      {
      write_mjpg_status_to_database();  
      system("sudo pkill mjpg_streamer");//Destroy rpi rtsp flux
      }
      else 
       printf("mjpg_streamer in idle state!!!");
   }
//********************Write infos to data base*****************//
 void write_temperature_to_database(float t)
  {
    //Ecriture dans la base de données   
     char *Querry = (char*) malloc(OFFSET_QUERRY_PREFIX);   
     sprintf(Querry, "UPDATE %s_parametre_visio SET temperature = '%d'",t,prefix); 
     if (mysql_query(conn, Querry))
      {
       fprintf(stderr, "%s\n", mysql_error(conn));
      } 
       //---------------Fin Ecriture-----------------//
     free(Querry);
     Querry=NULL;
  } 
 
 void write_door_status_to_database()
  { 
     //Ecriture dans la base de données
     char *Querry = (char*) malloc(OFFSET_QUERRY_PREFIX);   
     sprintf(Querry, "UPDATE %s_parametre_visio SET ouvrir_porte_visio = '0'",prefix);
      if (mysql_query(conn, Querry))
     {
       fprintf(stderr, "%s\n", mysql_error(conn));
      } 
       //---------------Fin Ecriture-----------------//
       free(Querry);//Free the allocated memory 
       Querry=NULL;    
  } 

void write_mjpg_status_to_database()
  { 
     //Ecriture dans la base de données
     char *Querry = (char*) malloc(OFFSET_QUERRY_PREFIX);   
     sprintf(Querry, "UPDATE %s_parametre_visio SET mjpg_streamer = '0'",prefix);
      if (mysql_query(conn, Querry))
     {
       fprintf(stderr, "%s\n", mysql_error(conn));
      } 
       //---------------Fin Ecriture-----------------//
       free(Querry);//Free the allocated memory 
       Querry=NULL;    
  } 
 
 void write_call_type_to_database(t_call_type history)
 {
  //Ecriture dans la base de données
     char *Querry = (char*) malloc(OFFSET_QUERRY_APPEL);   
     sprintf(Querry, "UPDATE %s_appels_visio SET type_appels_visio = '%d' order by id_appels_visio desc limit 1",prefix,history);
      if (mysql_query(conn, Querry))
     {
       fprintf(stderr, "%s\n", mysql_error(conn));
      } 
       //---------------Fin Ecriture-----------------//
       free(Querry);//Free the allocated memory 
       Querry=NULL;
 }

 void save_call_history_to_database()
  {
        int row_nbr = 0;
        //Enregistrement des appels dans la base de données
        char *Querry1 = (char*) malloc(OFFSET_QUERRY_APPEL);
        char *Querry2 = (char*) malloc(OFFSET_QUERRY_PREFIX);      
        sprintf(Querry1, "INSERT INTO %s_appels_visio (id_appels_visio,time_appels_visio,name_visio) VALUES('','%s','visiophone')",prefix,get_current_time()); 
        mysql_query(conn, Querry1);
        sprintf(Querry2, "SELECT * FROM %s_appels_visio",prefix);
        mysql_query(conn, Querry2);
        res = mysql_store_result(conn);
        row_nbr = mysql_num_rows(res);

        if (row_nbr == 101)//Maximum d'historique 100
         {
          sprintf(Querry2, "TRUNCATE TABLE %s_appels_visio",prefix); 
          mysql_query(conn, Querry2);
         }
        
        free(Querry1);//Free Allocated Memory
        free(Querry2);
        Querry1=NULL;
        Querry2=NULL;
       //---------------Fin Enregistrement-----------------//
   }
