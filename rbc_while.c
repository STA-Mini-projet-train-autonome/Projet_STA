#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>         
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


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


FILE * flog;

struct sockaddr_in adrserv, adrtrain; 

typedef struct {
    int pid;
    int numtrain;
} couple;

typedef struct {
    double positionEspace;
    double vitesse;
} etat;

typedef struct train {
    couple ensTrain[maxtrains]; //A CHANGER PAR UNE LISTE SIMPLEMENT CHAINEE
    int nbtrains; //nombre de trains 
} Ttrain;

int pid;
int tunnel[2*maxtrains][2];
etat position[maxtrains];


//Creation d'une structure qui sera passee en argument des fonctions servant a la creation des threads, et qui contient tous les parametres necessaires a l'execution de ces fonctions
typedef struct argument {
    int sizeaddr;
    struct sockaddr_in adrtrain;
    int se; // Ma socket d'ecoute
    int sd; //Ma socket de dialogue
    int numero; //Numéro du train actuel
    Ttrain trains;
   // Tthread threads;//Toutes les threads
} Targument;

int main(int argc, char * argv[]){
    //Section declaration des variables
    char message1[MAXCAR];
    char message2[MAXCAR];
    Targument arguments;
    arguments.sizeaddr = 0;
    arguments.numero = -1;
    int ret; //Gestion des erreurs de creation de threads
    //Section traitement

    // Phase 1 : Ouverture de la socket d'ecoute
    printf("\n Demarrage d'un Serveur INTERNET EN MODE CONNECTE !!! \n\n");
    arguments.se=socket(AF_INET, SOCK_STREAM, 0);//Creation de la socket d'ecoute
    CHECKERROR(arguments.se," \n\n Probleme d'ouverture de socket !!! \n\n"); //Verification que la socket a ete cree sans probleme
    //Definir l'adresse de la machine

    adrserv.sin_family=AF_INET;
    switch (argc){
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
    CHECKERROR(bind(arguments.se, (const struct sockaddr *)&adrserv,sizeof(adrserv)),"\n\nProblème de BIND !!!\n\n") ;

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
    arguments.trains.nbtrains=0;

    for(int i=0; i < maxtrains ; i++) {
        arguments.trains.ensTrain[i].pid=-1;
        arguments.trains.ensTrain[i].numtrain=-1;
    }

    char aux[MAXCAR];
    printf("Nombre de trains total : \n"); 
    gets(aux);
    int carlus;
    int caremis;
    int totalTrain;
    pid = 1; // On initialise la variable à 1 pour passer le premier tour de la boucle while.
    totalTrain=atoi(aux);

    // On attend d'abord que tous les trains se connectent
    while(arguments.trains.nbtrains<totalTrain && pid!=0){
        listen(arguments.se,5);
        // On accepte une demande d'ouverture de connexion. Dans cette version on ne recupere pas les coordonnees de l'appelant
        arguments.sizeaddr=sizeof(arguments.adrtrain);
        arguments.sd=accept(arguments.se, (struct sockaddr *)&arguments.adrtrain, (socklen_t*) &arguments.sizeaddr);
        //do{
        printf("Réception en cours de la socket %d !!! \n",arguments.sd);
        //getchar();
        carlus = read(arguments.sd,buf_reception,MAXCAR);
        if (!carlus) printf("Aucun caractère reçu !!! \n");
        
        else{
            printf("Réception effectuée de %d caractères !!! \n",carlus);
            char * pch;
            pch = strtok (buf_reception,";"); //On sépare la chaîne de caractère reçu selon le caractère ;      
            arguments.numero = atoi(&pch[0]);
            printf("Ordre : %d", arguments.numero);
            pch = strtok (NULL, ";");
            position[arguments.numero].positionEspace = atof(pch);
            printf(" positionEspace %f",position[arguments.numero].positionEspace);
            pch = strtok (NULL, ";");
            position[arguments.numero].vitesse = atof(pch);
            printf(" vitesse %f",position[arguments.numero].vitesse);
            
            arguments.trains.nbtrains = arguments.trains.nbtrains+1; //On incremente le nombre de train
            arguments.trains.ensTrain[arguments.numero].numtrain = arguments.numero; //On definit le train actuel
            printf("\n Connexion sur le serveur du train %s avec le port %d, le numero %d et la socket %d !!! \n\n", inet_ntoa(arguments.adrtrain.sin_addr), ntohs(arguments.adrtrain.sin_port),arguments.trains.ensTrain[arguments.numero].numtrain,arguments.sd);
            fprintf(flog,"\n Connexion sur le serveur du train %s avec le port %d,le numero %d et la socket %d !!! \n\n", inet_ntoa(arguments.adrtrain.sin_addr), ntohs(arguments.adrtrain.sin_port),arguments.trains.ensTrain[arguments.numero].numtrain,arguments.sd);
            
        }
    // On crée le tunnel 
    int tunnelPere;
    int tunnelFils;
    tunnelFils = pipe(tunnel[arguments.trains.ensTrain[arguments.numero].numtrain]); // Il faut que cette ligne soit impérativement placée avant le fork
    tunnelPere = pipe(tunnel[arguments.trains.ensTrain[arguments.numero].numtrain+maxtrains]);
    if (tunnelFils != 0|| tunnelPere != 0)
        printf("Problème avec la création des tunnels");
    /*  Pour écrire dans le tunnel, on utilise : ssize_t write(int entreeTube, const void *elementAEcrire, size_t nombreOctetsAEcrire); 
    Pour lire le tunnel, on utilise : ssize_t read(int sortieTube, void *elementALire, size_t nombreOctetsALire);*/
    pid=fork();
    if (pid!=0) { //Je suis dans le code du pere
        printf("\n Je suis le pere du processus de pid : %d \n\n", pid);
        close(tunnel[arguments.trains.ensTrain[arguments.numero].numtrain][0]);
        close(tunnel[arguments.trains.ensTrain[arguments.numero].numtrain+maxtrains][1]);   
        close(arguments.sd); // Le pere ferme la socket de dialogue pour ne pas interfere avec le fils
    }

    else { //Je suis dans le code du fils
        arguments.trains.ensTrain[arguments.numero].pid=getpid(); //On récupère le pid du train
        close(tunnel[arguments.trains.ensTrain[arguments.numero].numtrain][1]);
        close(tunnel[arguments.trains.ensTrain[arguments.numero].numtrain+maxtrains][0]);
        }
                
        position[0].positionEspace = 15;
        position[0].vitesse = 18;
    }
    while(1){
        position[0].positionEspace = position[0].positionEspace+2;
        position[0].vitesse = 18;
        //Reception de la position du train
        carlus=read(arguments.sd, buf_reception, MAXCAR);
        if (!carlus) printf("\n Il y a un probleme !!!\n");
        //Envoie de la position du train au RBC
    if(pid == 0)
    {        
        //printf("Nous sommes dans le fils (pid = %d).\nIl envoie le message suivant au père : \"%s\".\n\n\n", getpid(), buf_reception);
        write(tunnel[arguments.trains.ensTrain[arguments.numero].numtrain+maxtrains][1],buf_reception, MAXCAR);

        read(tunnel[arguments.trains.ensTrain[arguments.numero].numtrain][0], message1, MAXCAR);
        //printf("Nous sommes dans le fils (pid = %d).\nIl a reçu le message suivant du père : \"%s\".\n\n\n", getpid(), message1);
        caremis=write(arguments.sd, message1, strlen(message1)+1);

    }

    else
    {
        read(tunnel[arguments.trains.ensTrain[arguments.numero].numtrain+maxtrains][0], message2, MAXCAR);
        //printf("Nous sommes dans le père (pid = %d).\nIl a reçu le message suivant du fils : \"%s\".\n\n\n", getpid(), message2);
        char * pch;
        pch = strtok (message2,";"); //On sépare la chaîne de caractère reçu selon le caractère ;      
        position[arguments.numero].positionEspace = atof(pch);
        printf(" positionEspace %f \n",position[arguments.numero].positionEspace);
        pch = strtok (NULL, ";");
        position[arguments.numero].vitesse = atof(pch);
        printf(" vitesse %f \n",position[arguments.numero].vitesse);
        //printf("Nous sommes dans le père (pid = %d).\nIl envoie le message suivant au fils : \"%s\".\n\n\n", getpid(), message2);
        if(arguments.numero>0){
        sprintf(message2,"%f;%f",position[arguments.numero-1].positionEspace,position[arguments.numero-1].vitesse);
        write(tunnel[arguments.trains.ensTrain[arguments.numero].numtrain][1], message2, MAXCAR);
        }

    }
    
        sleep(1);
    }
    printf("Arret du programme principal !!!\n");

    //Fermeture des sockets
    close(arguments.sd);
    close(arguments.se);
    
    //Fermeture fichier
    fclose(flog);
    return 1;
}
