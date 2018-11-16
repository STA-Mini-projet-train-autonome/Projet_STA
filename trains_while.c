// Trains commit final
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>         
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>


#define LOCAL_ADDR_IP     "127.0.0.1"
#define REMOTE_ADDR_IP     "127.0.0.1"
#define REMOTE_PORT         8000
#define MAXCAR      80

// Numero de la balise GPS du train. A ADAPTER A LA SITUATION !!!
#define NUM_BALISE_GPS 12

#define CHECKERROR(code,msg)    if (code==-1) { \
                                    perror(msg);\
                                    exit(1);\
                                }
 
 
// Definition des variables globales
char buf_reception[MAXCAR];
char buf_emission[MAXCAR];

unsigned long tpsPrecedent=0; // Temps en ms
long positionPrecedent=0; // Position en mm

/* ------------------------------------------------------ Parametrage du port serie ---------------------------------------------------------- */

int set_interface_attribs(int fd, int speed)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }

    cfsetospeed(&tty, (speed_t)speed);
    cfsetispeed(&tty, (speed_t)speed);

    tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         /* 8-bit characters */
    tty.c_cflag &= ~PARENB;     /* no parity bit */
    tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

    /* setup for non-canonical mode */
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    /* fetch bytes as they become available */
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

/* ------------------------------------------------------ Fonction principale ---------------------------------------------------------- */

int main(int argc, char * argv[]){

    /* -------------------------------------------- OUVERTURE & PARAMETRAGE DU PORT SERIE ---------------------------------------------- */

    // Ouverture/parametrage du port serie
    char *portname = "/dev/ttyS0";
    int fd;
    fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        printf("Error opening %s: %s\n", portname, strerror(errno));
        return -1;
    }
    // Parametrage : baudrate 115200, 8 bits, pas de parite, 1 stop bit
    set_interface_attribs(fd, B115200);

    /* ------------------------------------ DECLARATION DES VARIABLES POUR LA LECTURE DES BALISES -------------------------------------- */

    int trameReception = 0;
    unsigned char trame_hexa [29];
    int verif=0;
    int trameRecue=0;
    long cor_x=0; // Position en x
    long cor_y=0; // Position en y
    unsigned char adressOfHedgehog=0;
    unsigned long timestamp; // Temps de la balise GPS
    unsigned char buf[2];
    int rdlen;
    unsigned char *p;
    unsigned char byte;
    
    /* ------------------------------------------- OUVERTURE & PARAMETRAGE DE LA SOCKET TCP -------------------------------------------- */
    
    // Declaration des variables locales
    int sd; //Ma socket de dialogue
    struct sockaddr_in adrclient;
    struct sockaddr_in adrserv;
    int carlus; //Nombre d'octets recu
    int caremis; //Nombre d'octets emis
    int t = 1; // Initialisation du temps en ms

    int sizeaddr; // Taille en octets de l'adresse
    
    char * pch;
    int ordre;
    double vitesse;
    double position;

    // Creation de la socket de dialogue
    sd=socket(AF_INET, SOCK_STREAM, 0);
    CHECKERROR(sd, "Probleme d'ouverture de socket \n");

    
    // Definir l'adresse du serveur eloigne
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

    // Demande de connexion au serveur
    CHECKERROR(connect(sd,(const struct sockaddr *) &adrserv, sizeof(adrserv)), "\n Echec connexion !!!\n\n");
    
    printf("\n Connexion sur le serveur %s avec le port %d et la socket %d !!! \n\n", inet_ntoa(adrserv.sin_addr), ntohs(adrserv.sin_port),sd);
    
    printf("Entrez l'ordre du train, sa position initiale ainsi que sa vitesse initiale sous le format 'ordre;position;vitesse' : ");
    gets(buf_emission);
    caremis = write(sd, buf_emission, strlen(buf_emission)+1);
    if (caremis) {
        printf("\n Message émis !!!  \n");
    }
    else printf("\n Il y a un problème !!!\n");
    
    pch = strtok (buf_emission, ";"); // On separe la chaîne de caractere recue selon le caractère ;
    ordre = atoi(&pch[0]);
    printf("   Ordre : %d\n", ordre);
    
    /* Simulation du train de tete cote train --> utile si on arrive a gerer plusieurs processus en parallele cote RBC */
    /*if (ordre == 0){ // Je suis dans le train tete
        pch = strtok (NULL, ";");
        position = atof(pch);
        printf("   Position dans l'espace : %f\n",position);
        
        pch = strtok (NULL, ";");
        vitesse = atof(pch);
        printf("   Vitesse : %f\n", vitesse);
        
        while(t<30){
            
            if (t<10){
                position += vitesse/t;
                vitesse++;
                sprintf(buf_emission, "%lf;%lf", position, vitesse);
                caremis = write(sd, buf_emission, strlen(buf_emission)+1);
                //buf_emission = NULL;
                printf("Vitesse increment!: '%lf' \n position: '%lf' \n", vitesse, position);
                carlus=read(sd,buf_reception, MAXCAR);
                printf("Donnees reçues : %s \n", buf_reception);
                //do sprintf(buf_emission,getchar());
                //while (buf_emission!='\n' && buf_emission!=EOF);
                usleep(1000000);
            }
            else if (t>=10 && t < 20){
                position += vitesse/t;
                sprintf(buf_emission, "%lf;%lf", position, vitesse);
                caremis = write(sd, buf_emission, strlen(buf_emission)+1);
                printf("Vitesse constant! '%lf' \n position: '%lf' \n", vitesse, position);
                carlus=read(sd,buf_reception, MAXCAR);
                printf("Donnees reçues : %s \n", buf_reception);
                //do sprintf(buf_emission,getchar());
                //while (buf_emission!='\n' && buf_emission!=EOF);
                //fflush(buf_emission);
                usleep(1000000);
            }
            else{
                position += vitesse/t;
                vitesse--;
                sprintf(buf_emission, "%lf;%lf", position, vitesse);
                caremis = write(sd, buf_emission, strlen(buf_emission)+1);
                printf("Vitesse decrement!: '%lf' \n position: '%lf' \n", vitesse, position);
                carlus=read(sd,buf_reception, MAXCAR);
                printf("Donnees reçues : %s \n", buf_reception);
                //do sprintf(buf_emission,getchar());
                //while (buf_emission!='\n' && buf_emission!=EOF);
                //fflush(buf_emission);
                usleep(1000000);
            }
            t++;
        }
    }*/
    //else{
        while(1){

            /* ------------------------------------------- RECUPERATION DE LA POSITION PAR BALISE GPS -------------------------------------------- */

            do {
                rdlen = read(fd, buf, sizeof(buf) - 1);
                if (rdlen > 0) {
                    unsigned char *p;
                    //printf("Read %d:", rdlen);
                    unsigned char byte;
                    for (p = buf; rdlen-- > 0; p++){
                        byte=*p;
                      //  printf(" %x \n ", byte);
                    }
                    if(trameReception>4){
                        trame_hexa[trameReception]=byte;
                        //printf("t :  %x \n", trame_hexa[trameReception]);
                        trameReception++;
                        if (trameReception==29){
                            // printf("=======Fin de la Trame=======\n");
                            trameReception=0;
                            verif=0;
                            trameRecue=1;
                        }
                    }
                    else {
                        /* Rechecherche du debut de la trame*/
                        switch (verif){
                            case 0:
                                if (byte == 0xff){
                                    // printf("=====Debut de trame possible=====\n");
                                    verif++;
                                    trame_hexa[trameReception]=byte;
                                    trameReception++;
                                    //printf("verif : %d\n",verif);
                                }
                                break;
                            case 1:
                                if (byte == 0x47){
                                    verif++;
                                    trame_hexa[trameReception]=byte;
                                    trameReception++;
                                    //printf("verif : %d\n",verif);
                                }
                                else {
                                    verif=0;
                                    //printf("Erreur de detection\n");
                                    trameReception=0;
                                }
                                break;
                            case 2:
                                if (byte == 0x11){
                                    verif++;
                                    trame_hexa[trameReception]=byte;
                                    trameReception++;
                                    //printf("verif : %d\n",verif);
                                }
                                else {
                                    verif=0;
                                    trameReception=0;
                                    //printf("Erreur de detection\n");
                                }
                                break;
                            case 3:
                                if (byte == 0x00){
                                    verif++;
                                    trame_hexa[trameReception]=byte;
                                    trameReception++;
                                    //printf("verif : %d\n",verif);
                                }
                                else {
                                    verif=0;
                                    trameReception=0;
                                    //printf("Erreur de detection\n");
                                }
                                break;
                            case 4:
                                if (byte == 0x16){
                                    verif++;
                                    trame_hexa[trameReception]=byte;
                                    trameReception++;
                                    //printf("verif : %d\n",verif);
                                }
                                else {
                                    trameReception=0;
                                    verif=0;
                                    //printf("Erreur de detection \n");
                                }
                                break;
                            default:
                                //printf("Erreur sur verif, reinitialisation\n");
                                trameReception=0;
                                verif=0;
                        }
                    }
                }
                else if (rdlen < 0) {
                            printf("Error from read: %d: %s\n", rdlen, strerror(errno));
                }
                //on vérifie que la balise est bien celle qu'on souhaite observer
                if (trameRecue && (trame_hexa[22]!=NUM_BALISE_GPS)){ 
                    printf("trame recue mais mauvaise balise");
                    trameRecue=0;
                }
            } while (!trameRecue);
            printf("Reception d'une trame complete : \n");
            int i;
            for (i=0;i<29; i++){
                printf ("%x ",trame_hexa[i]);
            }
            printf("\n");
            trameRecue=0;
            adressOfHedgehog=trame_hexa[22];
            timestamp=trame_hexa[5]+(trame_hexa[6]<<8)+(trame_hexa[7]<<16)+(trame_hexa[8]<<24);
            cor_x=trame_hexa[9]+(trame_hexa[10]<<8)+(trame_hexa[11]<<16)+(trame_hexa[12]<<24);
            cor_y=trame_hexa[13]+(trame_hexa[14]<<8)+(trame_hexa[15]<<16)+(trame_hexa[16]<<24);
            printf("\n Le train portant la balise %hhu a pour coordonnees x=%li et y=%li a t=%lu ms.\n\n",adressOfHedgehog,cor_x,cor_y,timestamp);

            /* -------------------------------------- FIN DE LA RECUPERATION DE LA POSITION PAR BALISE GPS ------------------------------------------ */

            /* -------- */
            /* Emission */
            /* -------- */
            
            printf("Envoi de la position dans l'espace et de la vitesse du train actuel\n");
            // Pour l'instant on considere qu'on se deplace uniquement selon l'axe des x, donc on utilise la coordonnee en x
            long position = cor_x; //en mm, recue de la balise
            double vitesse = (double)(position-positionPrecedent) / ((double) (timestamp - tpsPrecedent)); // Normalement recue de la simulation
            printf("   Position dans l'espace : %lu", position);
            printf("   Vitesse : %f", vitesse);
            
            sprintf(buf_emission, "%lf;%lf;%lu;%lu", (double)position, vitesse, timestamp, tpsPrecedent);
            caremis = write(sd,buf_emission, strlen(buf_emission)+1);
            tpsPrecedent = timestamp;
            positionPrecedent = position;
            if (!caremis) printf("Aucun caractère émis !!!\n");
            //else printf("%d caractères émis !!!\n",caremis);
            
            /* --------- */
            /* Reception */
            /* --------- */
            
            printf("Réception de la position dans l'espace et de la vitesse du train de devant\n");
            carlus = read(sd,buf_reception, MAXCAR);
 
            if (!carlus) printf("Aucun caractère reçu !!! \n");
            
            /* ------------------------------------------------------------ SIMULATION -------------------------------------------------------------- */
            // Mise a jour de la simulation avec les nouvelles valeurs
            
            else{
                char * pch;
                double positionTrainDevant, vitesseTrainDevant;
                pch = strtok (buf_reception,";"); // On separe la chaîne de caractere recue selon le caractere ;
                positionTrainDevant = atof(&pch[0]);
                printf("   Position du train devant : %lf\n",positionTrainDevant);
                pch = strtok(NULL, ";");
                vitesseTrainDevant = atof(pch);
                printf("   Vitesse du train devant : %lf\n",vitesseTrainDevant);
                // Envoyer les parametres en entree de la simulation
            }
        }
    //}

    /* ------------------------------------------------------------ Fin du programme -------------------------------------------------------------- */
    
    printf("\n\n Fin du dialogue cote client !!! Appuyez sur une touche ...\n\n");

    getchar();

    close(sd); // Fermeture de la socket de dialogue
}

