//RBC fonction main() premier commit
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

#define maxtrains      10

 
 
//Definition des variables globales
char buf_reception[MAXCAR];
char buf_emission[MAXCAR]; 
int flag_emission=0;
int flag_reception=0; 
int flag_fin=0;
int tunnel;

int *listeTrains[maxtrains];

void * receptionDonneesFils(); // tunnel
void * receptionDemandeConnexion();
void * receptionDonneesFilsTrainPrecedent(); // tunnel
void * receptionDonneesTrain();

void * envoiDonneesFilsDuTrainSuivant(); // tunnel
void * envoiDonneesRBC(); // tunnel
void * envoiDonneesTrain();

void * creationFils();

FILE * flog;

struct sockaddr_in adrserv, adrtrain; 

typedef struct {
    int pid;
    int numtrain;
} couple;

typedef struct train {
    couple ensTrain[maxtrains]; //A CHANGER PAR UNE LISTE SIMPLEMENT CHAINEE
    int nbtrains; //nombre de trains 
    } Ttrain;

//Creation d'une structure qui regroupe toutes les threads qui seront utilisees
typedef struct thread {
    pthread_t threadCreationFils;
    pthread_t threadReceptionDonneesFils;
    pthread_t threadReceptionDemandeConnexion;
    pthread_t threadReceptionDonneesFilsTrainPrecedent[maxtrains];
    pthread_t threadReceptionDonneesTrain[maxtrains];
} Tthread;
int pid;

int tunnel[2*maxtrains][2];


//Creation d'une structure qui sera passee en argument des fonctions servant a la creation des threads, et qui contient tous les parametres necessaires a l'execution de ces fonctions
typedef struct argument {
    int sizeaddr;
    struct sockaddr_in adrtrain;
    int se; // Ma socket d'ecoute
    int sd; //Ma socket de dialogue
    Ttrain trains;
    Tthread threads;//Toutes les threads
} Targument;

int main(int argc, char * argv[]){
    //Section declaration des variables
    
    Targument arguments;
    arguments.sizeaddr=0;
    int ret; //Gestion des erreurs de creation de threads

Ttrain trains;

int sizeaddr=0;

int tunnel;

int threadCreationFils;

    //PHASE 2 : Attente des connexions des trains

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

//PHASE 2 : Attente des connexions des trains

//Ouverture d'un fichier de logs

flog=fopen("serveur_stream_thread.log","a+");
if (!flog) {
    printf("Erreur d'ouverture du fichier de logs !!! \n\n");
    exit(-1);
            }
    
    //Si je suis ici c'est que j'ai affecte une adresse a ma socket d'ecoute
    //Et j'ai ouvert le fichier de logs

    //Initialisation des variables de memorisation des trains
    arguments.trains.nbtrains=-1;
    for(int i=0; i < maxtrains ; i++) arguments.trains.numtrain[i]=-1;

for(int i=0; i < maxtrains ; i++) {
    trains.ensTrain[i].pid=-1; 
    trains.ensTrain[i].numtrain=-1;
}

while (1)
{
   

    CHECKERROR(pthread_create(&threadCreationFils, NULL, creationFils(trains), NULL), "Erreur de creation du thread de création de fils!!!\n");
 

        // TRAITEMENT A ECRIRE ICI

        //Synchronisation des threads a la terminaison
        CHECKERROR(pthread_join(arguments.threads.threadCreationFils, NULL), "Erreur terminaison du thread de réception d'une demande de connexion !!!\n");
    CHECKERROR(pthread_join(arguments.threads.threadReceptionDonneesFilsTrainPrecedent[arguments.trains.numtrain[arguments.trains.nbtrains]], NULL), "Erreur terminaison du thread réception des données du train précédent !!!\n");
    CHECKERROR(pthread_join(arguments.threads.threadReceptionDonneesTrain[arguments.trains.numtrain[arguments.trains.nbtrains]], NULL), "Erreur terminaison du thread de réception des données du train !!!\n");
        CHECKERROR(pthread_join(arguments.threads.threadReceptionDonneesFils, NULL), "Erreur terminaison du thread de réception des données du fils !!!\n");
        CHECKERROR(pthread_join(arguments.threads.threadReceptionDemandeConnexion, NULL), "Erreur terminaison du thread de réception d'une demande de connexion !!!\n");
    }

    printf("Arret du programme principal !!!\n");

    //Fermeture des sockets
    close(arguments.sd);
    close(arguments.se);
    
    //Fermeture fichier
    fclose(flog);
    return 1;
}

void * creationFils(void * arg){
    //On se met a l'ecoute des demandes d'ouverture de connexion
    Targument *argumentsActuels = arg;
    listen(argumentsActuels->se,5);

    // On accepte une demande d'ouverture de connexion. Dans cette version on ne recupere pas les coordonnees de l'appelant
    argumentsActuels->sizeaddr=sizeof(argumentsActuels->adrtrain);
    argumentsActuels->sd=accept(argumentsActuels->se, (struct sockaddr *)&argumentsActuels->adrtrain, (socklen_t*) &argumentsActuels->sizeaddr);

    argumentsActuels->trains.nbtrains = argumentsActuels->trains.nbtrains+1; //On incremente le nombre de train
    argumentsActuels->trains.numtrain[argumentsActuels->trains.nbtrains] = argumentsActuels->trains.nbtrains; //On defint le train actuel

    printf("\n Connexion sur le serveur  du train %s avec le port %d, le numero %d et la socket %d !!! \n\n", inet_ntoa(argumentsActuels->adrtrain.sin_addr), ntohs(argumentsActuels->adrtrain.sin_port),argumentsActuels->trains.numtrain[argumentsActuels->trains.nbtrains],argumentsActuels->sd);
    fprintf(flog,"\n Connexion sur le serveur  du train %s avec le port %d,le numero %d et la socket %d !!! \n\n", inet_ntoa(argumentsActuels->adrtrain.sin_addr), ntohs(argumentsActuels->adrtrain.sin_port),argumentsActuels->trains.numtrain[argumentsActuels->trains.nbtrains],argumentsActuels->sd);
    // On crée le tunnel 
    int tunnelPere;
    int tunnelFils;
    tunnelFils = pipe(tunnel[trains.ensTrain[trains.nbtrains].numtrain]); // Il faut que cette ligne soit impérativement placée avant le fork
    tunnelPere = pipe(tunnel[trains.ensTrain[trains.nbtrains].numtrain+maxtrains]);
    if (tunnelFils != 0|| tunnelPere != 0)
        printf("Problème avec la création des tunnels");
    /*  Pour écrire dans le tunnel, on utilise : ssize_t write(int entreeTube, const void *elementAEcrire, size_t nombreOctetsAEcrire); 
    Pour lire le tunnel, on utilise : ssize_t read(int sortieTube, void *elementALire, size_t nombreOctetsALire);*/
    pid=fork();
    if (pid!=0) { //Je suis dans le code du pere
        printf("\n Je suis le pere du processus de pid : %d \n\n", pid);
        close(tunnel[trains.ensTrain[trains.nbtrains].numtrain][0]);
        close(tunnel[trains.ensTrain[trains.nbtrains].numtrain+maxtrains][1]);
        CHECKERROR(pthread_create(&threadReceptionDonneesFils, NULL, receptionDonneesFils, NULL), "Erreur de creation du thread de réception des données du fils !!! \n");
        CHECKERROR(pthread_create(&threadReceptionDemandeConnexion, NULL, receptionDemandeConnexion, NULL), "Erreur de creation du thread de réception d'une demande de connexion !!! \n");
        
        close(sd); // Le pere ferme la socket de dialogue pour ne pas interfere avec le fils
    }

    else { //Je suis dans le code du fils
    trains.ensTrain[trains.nbtrains].pid=pid; //On récupère le pid du train

    close(tunnel[trains.ensTrain[trains.nbtrains].numtrain][1]);
    close(tunnel[trains.ensTrain[trains.nbtrains].numtrain+maxtrains][0]);
    //Creation des threads
    CHECKERROR(pthread_create(&argumentsActuels->threads.threadReceptionDonneesFilsTrainPrecedent[argumentsActuels->trains.numtrain[argumentsActuels->trains.nbtrains]], NULL, receptionDonneesFilsTrainPrecedent, NULL), "Erreur de creation du thread de réception des données du train précédent!!!\n");
    CHECKERROR(pthread_create(&argumentsActuels->threads.threadReceptionDonneesTrain[argumentsActuels->trains.numtrain[argumentsActuels->trains.nbtrains]], NULL, receptionDonneesTrain, NULL), "Erreur de creation du thread de réception des données du train !!!\n");
    }
}

void * receptionDonneesFils( void * arg){
    char messageEcrire[MAXCAR];
    char messageLire[MAXCAR];
    if (pid!=0) { //Je suis dans le code du pere
        sprintf(messageEcrire, "Bonjour, fils. Je suis ton père !");
        printf("Nous sommes dans le père (pid = %d).\nIl envoie le message suivant au fils : \"%s\".\n\n\n", getpid(), messageEcrire);
        write(tunnel[trains.ensTrain[trains.nbtrains].numtrain+maxtrains][1], messageEcrire, MAXCAR);
    }
    else {
        read(tunnel[trains.ensTrain[trains.nbtrains].numtrain+maxtrains][0], messageLire, MAXCAR);
        printf("Nous sommes dans le fils (pid = %d).\nIl a reçu le message suivant du père : \"%s\".\n\n\n", getpid(), messageLire); 
    }
}   

//Fonction servant a etablir pour la premiere fois une connexion entre un train et le RBC
void * receptionDemandeConnexion( void * arg){
    Targument *argumentsActuels = arg;
    int carlus;
    do{
        printf("Réception en cours de la socket %d !!! \n",argumentsActuels->sd);
        getchar();
        carlus = read(argumentsActuels->sd,buf_reception,MAXCAR);
        if (!carlus) printf("Aucun caractère reçu !!! \n");
        else{
            flag_reception=1;
            printf("Réception effectuée de %d caractères !!! \n",carlus);
            // Ajout du numéro de train dans le tableau
        }
    }
    while (!flag_fin);
    printf("Fin du thread de réception de la demande de connexion !!! \n");
    pthread_exit(0);
} 

void * receptionDonneesFilsTrainPrecedent( void * arg){
    
} 

void * receptionDonneesTrain( void * arg){

} 

void * envoiDonneesFilsDuTrainSuivant( void * arg){
    
} 

void * envoiDonneesRBC( void * arg){
    
} 

void * envoiDonneesTrain( void * arg){
    
} 
