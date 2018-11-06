//RBC premier commit
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>         
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>



#define PORT_ECOUTE_SERVEUR     8000
#define MAXCAR      80

#define CHECKERROR(code,msg)    if (code==-1) { \
                                    perror(msg);\
                                    exit(1);\
                                    }

#define maxclients      10

 
 
 //Definition des variables globales
char buf_reception[MAXCAR];
char buf_emission[MAXCAR]; 
int flag_emission=0;
int flag_reception=0; 
int flag_fin=0;
int sd; //Ma socket de dialogue
 

//utilisation :
//serveur.exe <port_ecoute_serveur> <adresse_ip_serevur> 
                                    
int main(int argc, char * argv[])
{
//Section declaration des variables    
    
int se; //Ma socket d'ecoute

struct sockaddr_in adrserv, adrclient;

int sizeaddr=0;

pthread_t threadReception[maxclients]; //id thread pour la reception de donnees
pthread_t threadEmission[maxclients]; //id thread pour l'emission de donnees
pthread_t threadTraitement[maxclients]; //id thread pour le traitement des donnees

int ret; //Gestion des erreurs de creation de threads

typedef struct client {
int numclient[maxclients]; //numero du client TCP/IP
int nbclients; //nombre de clients 
} Tclient;

Tclient clients;

FILE * flog;

//Section traitement

// Phase 1 : Ouverture de la socket d'ecoute
printf("\n Demarrage d'un Serveur INTERNET EN MODE CONNECTE !!! \n\n");
se=socket(AF_INET, SOCK_STREAM, 0);//Creation de la socket d'ecoute
CHECKERROR(se," \n\n Probleme d'ouverture de socket !!! \n\n"); //Verification que la socket a ete cree sans probleme

//Definir l'adresse de la machine

adrserv.sin_family=AF_INET;
switch (argc)
{
    case 3:     //Utilisation : serveur_stream_thread.exe <port_ecoute_serveur> <adresse_ip_sereveur> 
                inet_aton(argv[2],&adrserv.sin_addr);
                //Et le numero de port
                adrserv.sin_port=htons(atoi(argv[1]));
                break;
    case 2:    //Utilisation : serveur.exe <port_ecoute_serveur> 
                adrserv.sin_addr.s_addr=INADDR_ANY;
                //Et le numero de port
                adrserv.sin_port=htons(atoi(argv[1]));
                break;
    case 1:    //Utilisation : serveur.exe
                adrserv.sin_addr.s_addr=INADDR_ANY;
                //Et le numero de port
                adrserv.sin_port=htons(PORT_ECOUTE_SERVEUR);
                break;
    default : printf("\nLa syntaxe d'utlisation est 'serveur_stream_thread.exe <port_ecoute_serveur> <adresse_ip_sereveur> ' !!! \n");
}

// On affecte une adresse a la socket d'ecoute du serveur      
CHECKERROR(bind(se, (const struct sockaddr *)&adrserv,sizeof(adrserv)),"\n\nProbleme de BIND !!!\n\n") ;



//PHASE 2 : Attente des connexions des clients

//Ouverture d'un fichier de logs

flog=fopen("serveur_stream_thread.log","a+");
if (!flog) {
    printf("Erreur d'ouverture du fichier de logs !!! \n\n");
    exit(-1);
            }
    
//Si je suis ici c'est que j'ai affecte une adresse a ma socket d'ecoute
//Et j'ai ouvert le fichier de logs

//Initialisation des varaibles de memorisation des clients
clients.nbclients=-1;

for(int i=0; i < maxclients ; i++) clients.numclient[i]=-1; 


while (1)
{
//On se met a l'ecoute des demandes d'ouverture de connexion
listen(se,5);

// On accepte une demande d'ouverture de connexion. Dans cette version on ne recupere pas les coordonnees de l'appelant
sizeaddr=sizeof(adrclient);
sd=accept(se, (struct sockaddr *)&adrclient, (socklen_t*) &sizeaddr);

clients.nbclients++; //On incremente le nombre de client
clients.numclient[clients.nbclients]=clients.nbclients; //On defint le client actuel

printf("\n Connexion sur le serveur  du client %s avec le port %d, le numero %d et la socket %d !!! \n\n", inet_ntoa(adrclient.sin_addr), ntohs(adrclient.sin_port),clients.numclient[clients.nbclients],sd);
fprintf(flog,"\n Connexion sur le serveur  du client %s avec le port %d,le numero %d et la socket %d !!! \n\n", inet_ntoa(adrclient.sin_addr), ntohs(adrclient.sin_port),clients.numclient[clients.nbclients],sd);

//Creation des threads
CHECKERROR(pthread_create(&threadReception[clients.numclient[clients.nbclients]], NULL, reception, NULL), "Erreur de creation du thread de Reception !!!\n");

CHECKERROR(pthread_create(&threadEmission[clients.numclient[clients.nbclients]], NULL, emission, NULL), "Erreur de creation du thread d'Emission !!!\n");

CHECKERROR(pthread_create(&threadTraitement[clients.numclient[clients.nbclients]], NULL, traitement, NULL), "Erreur de creation du thread d'Traitement !!! \n");

/*
while (1)
{
    usleep(0);
} */
//Synchronisation des threads a la terminaison

CHECKERROR(pthread_join(threadReception[clients.numclient[clients.nbclients]], NULL), "Erreur terminaison du thread Reception !!!\n");
CHECKERROR(pthread_join(threadEmission[clients.numclient[clients.nbclients]], NULL), "Erreur terminaison du thread Emission !!!\n");
CHECKERROR(pthread_join(threadTraitement[clients.numclient[clients.nbclients]], NULL), "Erreur terminaison du thread Traitement !!!\n");


}

printf("Arret du programme principal !!!\n");

//Fermeture des sockets
close(sd);
close(se);
//Fermeture fichier
fclose(flog);
return 1;
}
            