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
#include <errno.h>
#include <fcntl.h>
#include <termios.h>


#define LOCAL_ADDR_IP     "127.0.0.1"
#define REMOTE_ADDR_IP     "192.168.0.195"
#define REMOTE_PORT         8000
#define MAXCAR      80

// Numéro de la balise GPS du train. A ADAPTER A LA SITUATION !
#define NUM_BALISE_GPS 12

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

unsigned long tpsPrecedent=0;


//Parametrage du port serie
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



int main(int argc, char * argv[]){

    // ============= OUVERTURE & PARAMETRAGE PORT SERIE ==================
    //Ouverture/parametrage du port serie
    char *portname = "/dev/ttyS0";
    int fd;
    fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        printf("Error opening %s: %s\n", portname, strerror(errno));
        return -1;
    }
    // Parametrage : baudrate 115200, 8 bits, pas de parite, 1 stop bit
    set_interface_attribs(fd, B115200);

    //=======DECLARATION DES VARIABLES POUR LA LECTURE DES BALISES=======
    int trameReception = 0;
    unsigned char trame_hexa [29];
    int verif=0;
    int trameRecue=0;
    long cor_x=0;
    long cor_y=0;
    unsigned char adressOfHedgehog=0;
    unsigned long timestamp;
    unsigned char buf[2];
    int rdlen;
    unsigned char *p;
    unsigned char byte;
    // ========== OUVERTURE & PARAMETRAGE DE LA SOCKET TCP ???===========


    int sd;
    //Declaration des variables locales
    struct sockaddr_in adrclient;
    struct sockaddr_in adrserv;
    //int carlu ; //Nombre d'octets recu
    //int caremis; //Nombre d'octets emis

    int sizeaddr; // Taille en octets de l'adresse

    char * pch; 
    int ordre;
    double vitesse;
    double position;
    
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
        printf("\n Message émis !!! \n");
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

    pch = strtok (buf_emission,";"); //On sépare la chaîne de caractère reçu selon le caractère ;      
    ordre = atoi(&pch[0]);
    printf("Ordre : %d\n", ordre);

    int carlus;
    int t=1;

    if (ordre == 0){ //Je suis dans le train tete
        pch = strtok (NULL, ";");
        position = atof(pch);
        printf("PositionEspace : %f\n",position);

        pch = strtok (NULL, ";");
        vitesse = atof(pch);
        printf("Vitesse : %f\n", vitesse);

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
    }
    else{
        while(1){
            //Emission
            //printf("Position du train : \n");
            //gets(buf_emission);
            //
            }
    //=================== RECUPERATION DE LA POSITION PAR BALISE GPS ====================        
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

    //============== FIN DE LA RECUPERATION DE LA POSITION PAR BALISE GPS ===============

            printf("Envoi position et vitesse\n");
            //pour l'instant on considère qu'on se déplace uniquement selon l'ase des x donc on utilise la coordonnée en x
            double position = cor_x; //en mm
            double vitesse = cor_x / ((double) timestamp-tpsPrecedent);
            tpsPrecedent = timestamp;

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