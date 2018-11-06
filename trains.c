//Trains premier commit
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>         
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>


#define LOCAL_ADDR_IP     "172.31.65.163"
#define REMOTE_ADDR_IP     "172.31.65.163"
#define REMOTE_PORT         8000

#define MAXCAR      80

#define CHECKERROR(code,msg)    if (code==-1) { \
                                    perror(msg);\
                                    exit(1);\
                                    }
 
 
 //Definition des variables globales
char buf_reception[MAXCAR];
char buf_emission[MAXCAR]; 
int flag_emission=0; // A 1 indique que des data sont presentes pour etre emises
int flag_reception=0;  // A 1 indique que des data ont ete recues
int flag_fin=0; // A 1 indique l'arret du programme
int sd; //Ma socket de dialogue
 
//utilisation :
//client_stream_thread.exe <adresse_ip_serveur> <port_ecoute_serveur>
                                    

int main(int argc, char * argv[])
{
//Declaration des variables locales
struct sockaddr_in adrclient, adrserv;

//int carlu ; //Nombre d'octets recu
//int caremis; //Nombre d'octets emis

int sizeaddr; // Taille en octets de l'adresse

FILE * clientlog; //Fichier pour archiver les evenements

int threadReception; //identifiant du thread de reception
int threadEmission; //identifiant du thread de emission
int threadTraitement; //identifiant du thread de traitement

// Partie traitement 
printf("Demarage d'un CLIENT !!! \n");

//Creation de la socket
sd=socket(AF_INET, SOCK_STREAM, 0);
CHECKERROR(sd, "Probleme d'ouverture de socket \n");

//Specification partielle de l'adresse du client - la machine locale
/*
adrclient.sin_family=AF_INET;
adrclient.sin_port=htons(0); */


//Definir l'adresse du serveur eloigne

adrserv.sin_family=AF_INET;

switch (argc)  
{
    case 3 :  //cas 1 : Utilisation : client_stream_thread.exe  <adresse_ip_serveur> <port_ecoute_serveur>
                inet_aton(argv[1],&adrserv.sin_addr);
                //Et le numero de port
                adrserv.sin_port=htons(atoi(argv[2]));
                break; 
    case 2 :  //cas 2 : Utilisation : client_stream_thread.exe  <adresse_ip_serveur> 
                inet_aton(argv[1],&adrserv.sin_addr);
                //Et le numero de port defini localement
                adrserv.sin_port=htons(REMOTE_PORT);
                break; 
    case 1 :  //cas 3 : Utilisation : client_stream_thread.exe 
                inet_aton(REMOTE_ADDR_IP,&adrserv.sin_addr);
                //Et le numero de port
                adrserv.sin_port=htons(REMOTE_PORT);
                break;             
    default : printf("\n Cas imprevu !!! Gros bogue en ligne de commande !!!");
}
 
//Demande de connextion au serveur 
CHECKERROR(connect(sd,&adrserv, sizeof(adrserv)), "\n Echec connexion !!!\n\n");

// Ici cela signifie que la connexion a ete ouverte
// Ouverture du fichier de log des data du client
clientlog=fopen("client_stream_thread.log", "a+");
if (!clientlog) {
    printf("Echec d'ouverture du fichier de log du client !!!");
    exit(-1);
    }
    
    
printf("\n Connexion sur le serveur %s avec le port %d et la socket %d !!! \n\n", inet_ntoa(adrserv.sin_addr), ntohs(adrserv.sin_port),sd);
fprintf(clientlog,"\n Connexion sur le serveur  du client %s avec le port %d et la socket %d !!! \n\n", inet_ntoa(adrserv.sin_addr), ntohs(adrserv.sin_port),sd);

//Creation des threads
CHECKERROR(pthread_create(&threadReception, NULL, reception, NULL), "Erreur de creation du thread de Reception !!!\n");

CHECKERROR(pthread_create(&threadEmission, NULL, emission, NULL), "Erreur de creation du thread d'Emission !!!\n");

CHECKERROR(pthread_create(&threadTraitement, NULL, traitement, NULL), "Erreur de creation du thread d'Traitement !!! \n");


while (!flag_fin)
{
    usleep(50000); // Le delai est specifie en ms. Donc cela equivant à 50s
} 

//Synchronisation des threads a la terminaison

CHECKERROR(pthread_join(threadReception, NULL), "Erreur terminaison du thread Reception !!!\n");
CHECKERROR(pthread_join(threadEmission, NULL), "Erreur terminaison du thread Emission !!!\n");
CHECKERROR(pthread_join(threadTraitement, NULL), "Erreur terminaison du thread Traitement !!!\n");

printf("\n\n Fin du dialogue cote client !!! Appuyez sur une touche ...\n\n");

getchar();

fclose(clientlog);

close(sd);
}