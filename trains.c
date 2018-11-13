//Trains fonction main() premier commit
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

void * envoiDonneesFils( void * arg);
void * receptionDistance(void * arg);
void * simulation(void * arg);                                  

int main(int argc, char * argv[]){
    //Declaration des variables locales
    struct sockaddr_in adrclient, adrserv;

    //int carlu ; //Nombre d'octets recu
    //int caremis; //Nombre d'octets emis

    int sizeaddr; // Taille en octets de l'adresse

    FILE * clientlog; //Fichier pour archiver les evenements

    int threadEnvoiDonneesFils; //Thread dediee a l'envoi en continu des donnees (position courante et vitesse si possible)
    int threadReceptionDistance; //Thread dediee a le reception en continu de la distance qui separe le train actuel de son predecesseur

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
    CHECKERROR(connect(sd,&adrserv, sizeof(adrserv)), "\n Echec connexion !!!\n\n");

    // Ici cela signifie que la connexion a ete ouverte
    // Ouverture du fichier de log des data du client
    clientlog=fopen("trains.log", "a+");
    if (!clientlog) {
        printf("Echec d'ouverture du fichier de log du client !!!");
        exit(-1);
    }
    
    printf("\n Connexion sur le serveur %s avec le port %d et la socket %d !!! \n\n", inet_ntoa(adrserv.sin_addr), ntohs(adrserv.sin_port),sd);
    fprintf(clientlog,"\n Connexion sur le serveur du client %s avec le port %d et la socket %d !!! \n\n", inet_ntoa(adrserv.sin_addr), ntohs(adrserv.sin_port),sd);

    //Creation des threads
    CHECKERROR(pthread_create(&threadEnvoiDonneesFils, NULL, envoiDonneesFils, NULL), "Erreur de creation du thread d'envoi de donnee !!!\n");
    CHECKERROR(pthread_create(&threadReceptionDistance, NULL, receptionDistance, NULL), "Erreur de creation du thread de reception de distance !!!\n");

    while (!flag_fin){
        usleep(50000); // Le delai est specifie en ms. Donc cela equivaut à 50s
    }

    //Synchronisation des threads a la terminaison

    CHECKERROR(pthread_join(threadEnvoiDonneesFils, NULL), "Erreur terminaison du thread d'envoi de donnee !!!\n");
    CHECKERROR(pthread_join(threadReceptionDistance, NULL), "Erreur terminaison du thread de reception de distance !!!\n");

    printf("\n\n Fin du dialogue cote client !!! Appuyez sur une touche ...\n\n");

    getchar();

    fclose(clientlog);

    close(sd);
}

void * envoiDonneesFils( void * arg){
    int caremis ; //Nombre d'octets emis 
    do{
        usleep(50000);
        sprintf(buf_emission,"%s","J'envoie ma position et ma vitesse");
        printf("Envoi position et vitesse");
        if (flag_emission) {
            caremis=write(sd,buf_emission, strlen(buf_emission)+1);
            flag_emission=0;
        }
    }
    while ((strcmp(tolower(buf_emission),"fin")) || flag_fin);
    printf("Fin du thread envoiDonneesFils !!!\n");
    pthread_exit(0);
}

void * receptionDistance( void * arg){
    int carlus ; //Nombre d'octets recu
    do{
    carlus=read(sd,buf_reception, MAXCAR);
    printf("Donnees reçues : %s", buf_reception);
    if (!carlus) printf("Aucun caractere RECU !!! \n");
        else {
            flag_reception=1;
            //printf("Reception effectuee de %d caracteres !!! \n",carlus);
        }
        // Mise à jour de la simulation avec les nouvelles valeurs
    }
    while ((strcmp(tolower(buf_reception),"fin")) || flag_fin);
    printf("Fin du thread repectionDistance !!!\n");
    pthread_exit(0);
}
