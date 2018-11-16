// RBC commit final
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

#define CHECKERROR(code,msg)    if (code == -1) { \
                                    perror(msg);\
                                    exit(1);\
                                    }

#define maxtrains      10

 
 
//Definition des variables globales
char buf_reception[MAXCAR];
char buf_emission[MAXCAR];

FILE * flog;

struct sockaddr_in adrserv, adrtrain; 

// Structure representant un train par son numero (ordre dans la file des trains) et par le PID du processus associé
typedef struct {
    int pid;
    int numtrain;
} couple;

// Structure representant l'etat d'un train par sa position actuelle dans l'espace et sa vitesse actuelle
typedef struct {
    double positionEspace;
    double vitesse;
} etat;

// Structure representant un train
typedef struct train {
    couple ensTrain[maxtrains]; // tableau dont la case d'indice i (0<=i<maxtrains) contient le couple (pid, numtrain) du train de numtrain = i;
    int nbtrains; //nombre de trains 
} Ttrain;

int pid;
int tunnel[2*maxtrains][2]; // Tunnel servant a la communication entre un pere et son fils
etat position[maxtrains]; // Tableau des etats (position dans l'espace et vitesse) de chaque train connecte


//Creation d'une structure qui au depart, devait etre passee en argument des fonctions servant a la creation des threads, et qui contient tous les parametres necessaires a l'execution de ces fonctions.
typedef struct argument {
    int sizeaddr;
    struct sockaddr_in adrtrain;
    int se; // Ma socket d'ecoute
    int sd; //Ma socket de dialogue
    int numero; //Numero du train actuel
    Ttrain trains;
} Targument;


int main(int argc, char * argv[]){
    
    /* ------------------------------------------------------ Section declaration des variables ---------------------------------------------------------- */
    
    char message1[MAXCAR];
    char message2[MAXCAR];
    Targument arguments;
    arguments.sizeaddr = 0;
    arguments.numero = -1;
    
    /* ------------------------------------------------------------- Section traitement ------------------------------------------------------------------ */

    /* ----------------------------------------- */
    /* PHASE 1 : Ouverture de la socket d'ecoute */
    /* ----------------------------------------- */
    
    printf("\n Demarrage d'un Serveur INTERNET EN MODE CONNECTE !!! \n\n");
    arguments.se=socket(AF_INET, SOCK_STREAM, 0); // Creation de la socket d'ecoute
    CHECKERROR(arguments.se, " \n\n Probleme d'ouverture de socket !!! \n\n"); // Verification que la socket a ete cree sans probleme
    
    // Definir l'adresse de la machine
    adrserv.sin_family = AF_INET;
    switch (argc){
        case 3:     // Utilisation : serveur_stream_thread.exe <port_ecoute_serveur> <adresse_ip_sereveur>
            inet_aton(argv[2], &adrserv.sin_addr);
            // et le numero de port
            adrserv.sin_port = htons(atoi(argv[1]));
            break;
        case 2:    // Utilisation : serveur.exe <port_ecoute_serveur>
            adrserv.sin_addr.s_addr = INADDR_ANY;
            // et le numero de port
            adrserv.sin_port = htons(atoi(argv[1]));
            break;
        case 1:    // Utilisation : serveur.exe
            adrserv.sin_addr.s_addr = INADDR_ANY;
            // et le numero de port
            adrserv.sin_port = htons(PORT_ECOUTE_SERVEUR);
            break;
        default : printf("\nLa syntaxe d'utlisation est 'serveur_stream_thread.exe <port_ecoute_serveur> <adresse_ip_sereveur> ' !!! \n");
    }

    // On affecte une adresse a la socket d'ecoute du serveur
    CHECKERROR(bind(arguments.se, (const struct sockaddr *)&adrserv,sizeof(adrserv)),"\n\nProblème de BIND !!!\n\n") ;

    /* ------------------------------------------- */
    /* PHASE 2 : Attente des connexions des trains */
    /* ------------------------------------------- */
    
    // Ouverture d'un fichier de logs
    flog=fopen("serveur_stream_thread.log","a+");
    if (!flog) {
        printf("Erreur d'ouverture du fichier de logs !!! \n\n");
        exit(-1);
    }
    
    // Si je suis ici c'est que j'ai affecte une adresse a ma socket d'ecoute
    // et que j'ai ouvert le fichier de logs

    // Initialisation des variables de memorisation des trains
    arguments.trains.nbtrains = 0;

    for(int i=0; i < maxtrains ; i++) {
        arguments.trains.ensTrain[i].pid = -1;
        arguments.trains.ensTrain[i].numtrain = -1;
    }

    char aux[MAXCAR];
    // On demande a l'utilisateur le nombre total de trains qui vont se connecter
    printf("\n\nEntrez le nombre total de trains qui vont se connecter : \n");
    gets(aux);
    int carlus;
    int caremis;
    int totalTrain;
    pid = 1; // On initialise la variable a 1 pour passer le premier tour de la boucle while.
    totalTrain = atoi(aux);

    // On attend d'abord que tous les trains se connectent
    while((arguments.trains.nbtrains < totalTrain) && (pid != 0)){
        listen(arguments.se,5);
        // On accepte une demande d'ouverture de connexion. Dans cette version on ne recupere pas les coordonnees de l'appelant
        arguments.sizeaddr = sizeof(arguments.adrtrain);
        arguments.sd = accept(arguments.se, (struct sockaddr *)&arguments.adrtrain, (socklen_t*) &arguments.sizeaddr);
        printf("Réception en cours de la socket %d !!! \n", arguments.sd);
        carlus = read(arguments.sd, buf_reception,MAXCAR);
        if (!carlus) printf("Aucun caractère reçu !!! \n");
        
        else{
            printf("Réception effectuée de %d caractères !!! \n",carlus);
            char * pch;
            pch = strtok (buf_reception,";"); //On sépare la chaîne de caractère reçu selon le caractère ;      
            arguments.numero = atoi(&pch[0]);
            printf("   Ordre : %d\n", arguments.numero);
            pch = strtok (NULL, ";");
            position[arguments.numero].positionEspace = atof(pch);
            printf("   Position dans l'espace : %f\n",position[arguments.numero].positionEspace);
            pch = strtok (NULL, ";");
            position[arguments.numero].vitesse = atof(pch);
            printf("   Vitesse : %f\n",position[arguments.numero].vitesse);
            
            arguments.trains.nbtrains = arguments.trains.nbtrains+1; //On incremente le nombre de train
            arguments.trains.ensTrain[arguments.numero].numtrain = arguments.numero; //On definit le train actuel
            printf("\n Connexion sur le serveur du train %s avec le port %d, le numero %d et la socket %d !!! \n\n", inet_ntoa(arguments.adrtrain.sin_addr), ntohs(arguments.adrtrain.sin_port),arguments.trains.ensTrain[arguments.numero].numtrain,arguments.sd);
            fprintf(flog,"\n Connexion sur le serveur du train %s avec le port %d,le numero %d et la socket %d !!! \n\n", inet_ntoa(arguments.adrtrain.sin_addr), ntohs(arguments.adrtrain.sin_port),arguments.trains.ensTrain[arguments.numero].numtrain,arguments.sd);
            
        }
        // On cree le tunnel
        int tunnelPere;
        int tunnelFils;
        tunnelFils = pipe(tunnel[arguments.trains.ensTrain[arguments.numero].numtrain]); // Il faut que cette ligne soit impérativement placée avant le fork
        tunnelPere = pipe(tunnel[arguments.trains.ensTrain[arguments.numero].numtrain+maxtrains]);
        if (tunnelFils != 0|| tunnelPere != 0)
            printf("Problème avec la création des tunnels");
        /*  Pour écrire dans le tunnel, on utilise : ssize_t write(int entreeTube, const void *elementAEcrire, size_t nombreOctetsAEcrire);
            Pour lire le tunnel, on utilise : ssize_t read(int sortieTube, void *elementALire, size_t nombreOctetsALire); */
        pid=fork();
        if (pid!=0) { //J e suis dans le code du pere
            printf("\n Je suis le pere du processus de pid : %d \n\n", pid);
            close(tunnel[arguments.trains.ensTrain[arguments.numero].numtrain][0]);
            close(tunnel[arguments.trains.ensTrain[arguments.numero].numtrain+maxtrains][1]);
            close(arguments.sd); // Le pere ferme la socket de dialogue pour ne pas interferer avec le fils
        }

        else { // Je suis dans le code du fils
            arguments.trains.ensTrain[arguments.numero].pid=getpid(); // On recupere le pid du train
            close(tunnel[arguments.trains.ensTrain[arguments.numero].numtrain][1]);
            close(tunnel[arguments.trains.ensTrain[arguments.numero].numtrain+maxtrains][0]);
        }
        
        // On demande la position et la vitesse du train de tete
        printf("Entrez la position dans l'espace du train de tête : \n");
        gets(aux);
        position[0].positionEspace = atof(aux);
        printf("Entrez la vitesse du train de tête : \n");
        gets(aux);
        position[0].vitesse = atof(aux);
    }
    
    unsigned long t = 1; // Temps en ms
    while(1){

        unsigned long timestamp; // Temps en ms envoye par le train depuis la trame de la balise GPS
        unsigned long tempsPrecedent; // Temps precedent en ms envoye par le train
        
        // Reception de la position du train
        carlus=read(arguments.sd, buf_reception, MAXCAR);
        if (!carlus) printf("\n Il y a un probleme !!!\n");
        
        // Envoi de l'etat du train depuis le fils au RBC
        if(pid == 0){
            printf("Nous sommes dans le fils (pid = %d).\nEnvoi de la position dans l'espace du train au RBC", getpid());
            write(tunnel[arguments.trains.ensTrain[arguments.numero].numtrain+maxtrains][1],buf_reception, MAXCAR);
            read(tunnel[arguments.trains.ensTrain[arguments.numero].numtrain][0], message1, MAXCAR);
            caremis=write(arguments.sd, message1, strlen(message1)+1);
        }
        // Reception de l'etat du train dans le RBC depuis le fils
        else{
            printf("Nous sommes dans le père (pid = %d).\n", getpid());
            read(tunnel[arguments.trains.ensTrain[arguments.numero].numtrain+maxtrains][0], message2, MAXCAR);
            
            // Traitement de la chaine de caracteres recue
            char * pch;
            pch = strtok (message2,";"); // On separe la chaîne de caractere recu selon le caractere
            position[arguments.numero].positionEspace = atof(pch);
            printf("   Position dans l'espace : %f \n",position[arguments.numero].positionEspace);
            pch = strtok (NULL, ";");
            position[arguments.numero].vitesse = atof(pch);
            printf("   Vitesse : %f \n",position[arguments.numero].vitesse);
            pch = strtok(NULL, ";");
            timestamp = atof(pch);
            pch = strtok(NULL, ";");
            tempsPrecedent = atof(pch);
            
            // On envoie aux fils des trains qui ne sont pas en tete, l'etat du train de devant (position dans l'espace et vitesse)
            if(arguments.numero>0){
                sprintf(message2,"%f;%f",position[arguments.numero-1].positionEspace,position[arguments.numero-1].vitesse);
                write(tunnel[arguments.trains.ensTrain[arguments.numero].numtrain][1], message2, MAXCAR);
            }
            // On envoie au fils du train de tete que la recepetion a bien ete effectuee
            else{
                sprintf(message2,"OK;OK");
                write(tunnel[arguments.trains.ensTrain[arguments.numero].numtrain][1], message2, MAXCAR);
            }
            
            // On simule le train de tete dans le RBC avec une courbe de vitesse en trapeze
            if ((t < 10000.0) && (tempsPrecedent != 0) && ((timestamp-tempsPrecedent) > 0)){
                position[0].positionEspace += position[0].vitesse/((double)(timestamp-tempsPrecedent)*0.001);
                position[0].vitesse++;
                t = t + timestamp-tempsPrecedent; //en ms
            }
            else if ((t >= 10000.0) && (t < 20000.0) && (tempsPrecedent != 0) && ((timestamp-tempsPrecedent) > 0)){
                position[0].positionEspace += position[0].vitesse/((double)(timestamp-tempsPrecedent)*0.001);
                t = t + timestamp-tempsPrecedent; //en ms
            }
            else if ((position[0].vitesse > 0) && (tempsPrecedent != 0) && ((timestamp-tempsPrecedent) > 0)){
                position[0].positionEspace += position[0].vitesse/((double)(timestamp-tempsPrecedent)*0.001);
                position[0].vitesse--;
                t = t + timestamp-tempsPrecedent; //en ms
            }
            else;
        }
        sleep(1);
    }
    
    /* ------------------------------------------------------ Section fin du programme ---------------------------------------------------------- */
    
    printf("Arrêt du programme principal !!!\n");

    // Fermeture des sockets
    close(arguments.sd);
    close(arguments.se);
    
    // Fermeture du fichier de log
    fclose(flog);
    return 1;
}
