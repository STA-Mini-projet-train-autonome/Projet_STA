//Trains fonction main()_v2 second commit
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>         
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>


#define LOCAL_ADDR_IP     "192.168.0.195"
#define REMOTE_ADDR_IP     "192.168.0.155"
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
//Ma socket de dialogue

unsigned long tpsPrecedent;

int main(int argc, char * argv[]){
    int sd; 
    //Declaration des variables locales
    struct sockaddr_in adrclient;
    struct sockaddr_in adrserv;
    //int carlu ; //Nombre d'octets recu
    //int caremis; //Nombre d'octets emis

    int sizeaddr; // Taille en octets de l'adresse
    
  //  FILE * clientlog; //Fichier pour archiver les evenements

    //Creation de la socket
    sd=socket(AF_INET, SOCK_STREAM, 0);
    CHECKERROR(sd, "Probleme d'ouverture de socket \n");

    
    //Definir l'adresse du serveur eloigne
    adrserv.sin_family=AF_INET;

    switch (argc){
        case 3 :  //cas 1 : Utilisation : trains.exe  <adresse_ip_serveur> <port_ecoute_serveur>
            inet_aton(argv[1],&adrserv.sin_addr);
            //Et le numero de port
            adrserv.sin_port=htons(atoi(argv[2]));
            break;
        case 2 :  //cas 2 : Utilisation : trains.exe  <adresse_ip_serveur>
            inet_aton(argv[1],&adrserv.sin_addr);
            //Et le numero de port defini localement
            adrserv.sin_port=htons(REMOTE_PORT);
            break;
        case 1 :  //cas 3 : Utilisation : trains.exe
            inet_aton(REMOTE_ADDR_IP,&adrserv.sin_addr);
            //Et le numero de port
            adrserv.sin_port=htons(REMOTE_PORT);
            break;
        default : printf("\n Cas imprevu !!! Gros bogue en ligne de commande !!!");
    }

    //Demande de connexion au serveur
    CHECKERROR(connect(sd,(const struct sockaddr *) &adrserv, sizeof(adrserv)), "\n Echec connexion !!!\n\n");
    
    printf("\n Connexion sur le serveur %s avec le port %d et la socket %d !!! \n\n", inet_ntoa(adrserv.sin_addr), ntohs(adrserv.sin_port),sd);
    
    int caremis; //Nombre d'octets emis
    printf("Entrez l'ordre du train, sa position initiale ainsi que sa vitesse initiale sous le format 'ordre;position;vitesse' : ");
    gets(buf_emission);
    caremis = write(sd, buf_emission, strlen(buf_emission)+1);
    if (caremis) {
        printf("\n Message émis !!!  \n");
    }
    else printf("\n Il y a un problème !!!\n");

    // Ici cela signifie que la connexion a ete ouverte
    // Ouverture du fichier de log des data du client
    /*clientlog=fopen("trains.log", "a+");
    if (!clientlog) {
        printf("Echec d'ouverture du fichier de log du client !!!");
        exit(-1);
    }*/
    
   // fprintf(clientlog,"\n Connexion sur le serveur du client %s avec le port %d et la socket %d !!! \n\n", inet_ntoa(adrserv.sin_addr), ntohs(adrserv.sin_port),sd);
    
    
    int carlus;
    while(1){
        //Emission
        //printf("Position du train : \n");
        //gets(buf_emission);
        //
        
        printf("Envoi position et vitesse\n");
        double position = 11.763;
        double vitesse = 13.987;
        sprintf(buf_emission,"%lf;%lf",position,vitesse);
        caremis=write(sd,buf_emission, strlen(buf_emission)+1);
            if (!caremis) printf("Aucun caractère émis !!!\n");
            //else printf("%d caractères émis !!!\n",caremis);
        //Reception
    
        //printf("Réception en cours sur la socket %d !!! \n", sd);
        carlus=read(sd,buf_reception, MAXCAR);
        printf("Donnees reçues : %s \n", buf_reception);
        //printf("Essai1\n");
        if (!carlus) printf("Aucun caractère reçu !!! \n");
        // Mise à jour de la simulation avec les nouvelles valeurs
        else{
            char * pch;
            double positionTrainDevant, vitesseTrainDevant;
            pch = strtok (buf_reception,";"); //On sépare la chaîne de caractère reçu selon le caractère ;
            positionTrainDevant = atof(&pch[0]);
            printf("Position du train devant : %lf\n",positionTrainDevant);
            pch = strtok(NULL, ";");
            vitesseTrainDevant = atof(pch);
            printf("Vitesse du train devant : %lf\n",vitesseTrainDevant);
        }
    }
    //Synchronisation des threads a la terminaison


    printf("\n\n Fin du dialogue cote client !!! Appuyez sur une touche ...\n\n");

    getchar();

   // fclose(clientlog);

    close(sd);
}
