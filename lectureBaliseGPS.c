#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdbool.h>


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

int main()
{
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

    int trameReception = 0;
    unsigned char trame_hexa [29];
    int verif=0;
    int trameRecue=0;
    long cor_x=0;
    long cor_y=0;
    unsigned char adressOfHedgehog=0;
    unsigned long timestamp;
    /* simple noncanonical input */
    do {
        unsigned char buf[2];
        int rdlen;

        rdlen = read(fd, buf, sizeof(buf) - 1);
        if (rdlen > 0) {
            unsigned char *p;
            printf("Read %d:", rdlen);
	    unsigned char byte;
            for (p = buf; rdlen-- > 0; p++){
		byte=*p;
                printf(" %x \n ", byte);
		if(trameReception>4){
			trame_hexa[trameReception]=byte;
			//printf("t :  %x \n", trame_hexa[trameReception]);
			trameReception++;
		        if (trameReception==29){
				printf("=======Fin de la Trame=======\n");
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
					printf("=====Debut de trame possible=====\n");
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
					printf("Erreur de detection");
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
					printf("Erreur de detection");
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
                                        printf("Erreur de detection");
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
                                        printf("Erreur de detection");
                                }
                                break;
			default:
				printf("Erreur sur verif, reinitialisation");
				trameReception=0;
				verif=0;
			}
		}
	}

        } else if (rdlen < 0) {
            printf("Error from read: %d: %s\n", rdlen, strerror(errno));
        }
	if(trameRecue){
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

	}
    } while (1);
}

