#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <pthread.h>

#include <unistd.h>
#include <iostream>

#include <string.h>
#include <errno.h>

#include "metis.h"
#include "metis.cc"
#include "HermesProxy.h"
#include "HermesProxy.cc"
#include "alsathreads.c"
#include "alsathreads.h"

#include <getopt.h>


#include <sys/ioctl.h>
#include <termios.h>



bool kbhit()
{
        termios term;
        tcgetattr(0, &term);

        termios term2 = term;
        term2.c_lflag &= ~ICANON;
        tcsetattr(0, TCSANOW, &term2);

        int byteswaiting;
        ioctl(0, FIONREAD, &byteswaiting);

        tcsetattr(0, TCSANOW, &term);

        return byteswaiting > 0;
}



HermesProxy* Hermes;

struct hpsdrinfos
{
        int frx[9]={-1,-1,-1,-1,-1,-1,-1,-1};
        char *adrx[9]={NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL}; //plughw:CARD=loopTest1,DEV=0 dmix:CARD=PCH,DEV=0
        int nRX=0;
        int samplerate=48000;
        char *hpsdrMAC=(char *)"*";
        char *interface=(char *)"eth0";
        unsigned int hermesbuffersize=4096;
		unsigned int alsabuffersize=128*1024;
} hpsdroptions;


void prexample(char *argv[]){
        fprintf (stderr,"\033[0;32m");
        fprintf (stderr,"%s --interface=wlo1 --samplerate=48000 --adrx1=plughw:CARD=PCH,DEV=0 --frx1=14074000 --adrx2=plughw:CARD=loopTest1,DEV=0 --frx2=7200000 --nRX=2\n",argv[0]);
        fprintf (stderr,"--interface is the network interface where is your receiver connected.");
		fprintf (stderr,"--samplerate is the general sample rate of your receiver.Default is %u.", hpsdroptions.samplerate);
		fprintf (stderr,"--adrx1 is the ouput alsa sound device for rx1, adrx2 for rx2 etc...");
		fprintf (stderr,"--frx1 is the frequency to set for adrx1 in hz, frx2 for adrx2, etc...");
		fprintf (stderr,"--nRX is your rx count, if you use 2 receiver set to 2.");
		fprintf (stderr,"--hermesbuffersize is the hermes buffer size. Default is %u.", hpsdroptions.hermesbuffersize);
		fprintf (stderr,"--alsabuffersize is the alsa output buffer size. Default is %u.", hpsdroptions.alsabuffersize);
		fprintf (stderr,"\033[0m");
}

int main(int argc, char *argv[])
{

        int c;
        int digit_optind = 0;




        static struct option long_options[] = {
                {"frx1",required_argument,0,0},{"adrx1",required_argument,0,0},
                {"frx2",required_argument,0,0},{"adrx2",required_argument,0,0},
                {"frx3",required_argument,0,0},{"adrx3",required_argument,0,0},
                {"frx4",required_argument,0,0},{"adrx4",required_argument,0,0},
                {"frx5",required_argument,0,0},{"adrx5",required_argument,0,0},
                {"frx6",required_argument,0,0},{"adrx6",required_argument,0,0},
                {"frx7",required_argument,0,0},{"adrx7",required_argument,0,0},
                {"frx8",required_argument,0,0},{"adrx8",required_argument,0,0},
                {"nRX",required_argument,0,0},
                {"samplerate",required_argument,0,0},
                {"hpsdrMAC",required_argument,0,0},
                {"interface",required_argument,0,0},
                {"hermesbuffersize",required_argument,0,0},
				{"alsabuffersize",required_argument,0,0},
                {0,0,0,0}
        };

        while (1) {
                int this_option_optind = optind ? optind : 1;
                int option_index = 0;


                c = getopt_long(argc, argv, "",
                                long_options, &option_index);
                if (c == -1)
                        break;

                switch (c) {
                case 0:
                        printf("option %s", long_options[option_index].name);
                        if (optarg) {
                                printf(" is %s\n", optarg);
                                if(long_options[option_index].name[0] == 'f') {hpsdroptions.frx[(int)(long_options[option_index].name[3]-'0')]=atoi(optarg);}
                                else if(long_options[option_index].name[0] == 'a') {hpsdroptions.adrx[(int)(long_options[option_index].name[4]-'0')]=optarg;}
                                else if(strcmp(long_options[option_index].name, "nRX")==0) {hpsdroptions.nRX=atoi(optarg);}
                                else if(strcmp(long_options[option_index].name, "samplerate")==0) {hpsdroptions.samplerate=atoi(optarg);}
                                else if(strcmp(long_options[option_index].name, "hpsdrMAC")==0) {hpsdroptions.hpsdrMAC=optarg;}
                                else if(strcmp(long_options[option_index].name, "interface")==0) {hpsdroptions.interface=optarg;}
                                else if(strcmp(long_options[option_index].name, "hermesbuffersize")==0) {hpsdroptions.hermesbuffersize=atoi(optarg);}
								else if(strcmp(long_options[option_index].name, "alsabuffersize")==0) {hpsdroptions.alsabuffersize=atoi(optarg);}
                        }
                        break;

                case '0':
                case '1':
                case '2':
                        if (digit_optind != 0 && digit_optind != this_option_optind)
                                printf("digits occur in two different argv-elements.\n");
                        digit_optind = this_option_optind;
                        printf("option %c\n", c);
                        break;

                case '?':
                        break;

                default:
                        printf("?? getopt returned character code 0%o ??\n", c);
                }
        }

        if (optind < argc) {
                printf("non-option elements: ");
                while (optind < argc)
                        printf("%s ", argv[optind++]);
                printf("\n");
        }

        if(hpsdroptions.nRX==0) {
                fprintf (stderr,"\033[1;31m");
                fprintf (stderr, "please give the number of receivers --nRX=\n");
                fprintf (stderr,"\033[0m");
                prexample(argv);
                exit(1);
        }

        for(int i=1; i<=hpsdroptions.nRX; ++i) {
                if(hpsdroptions.adrx[i] == NULL || hpsdroptions.frx[i] == -1) {
                        fprintf (stderr,"\033[1;31m");
                        fprintf (stderr, "rx%d not initialized, miss frequency or alsa device\n",i);
                        fprintf (stderr,"\033[0m");
                        exit(1);
                }
        }


        Hermes = new HermesProxy(hpsdroptions.frx[1],hpsdroptions.frx[2],hpsdroptions.frx[3],hpsdroptions.frx[4],
                                 hpsdroptions.frx[5], hpsdroptions.frx[6],hpsdroptions.frx[7], hpsdroptions.frx[8],
                                 0, 0,
                                 0, 0, 0,
                                 '0', hpsdroptions.samplerate, hpsdroptions.interface,
                                 "0xF8", 0, 0,
                                 0, 0, 0, hpsdroptions.nRX,
                                 hpsdroptions.hpsdrMAC);

        pthread_t alsadeviceth[hpsdroptions.nRX];

        int NumRx = Hermes->NumReceivers;
        int SamplesPerRx = Hermes->USBRowCount[NumRx-1];

        struct thargs *argvvv[NumRx];
        //short *output_items[NumRx][SamplesPerRx][2];

        pthread_attr_t attr;
        struct sched_param param;
        pthread_attr_init (&attr);
        pthread_attr_getschedparam (&attr, &param);
        (param.sched_priority)++;
        pthread_attr_setschedparam (&attr, &param);

        for(int i=1; i<=NumRx; ++i) {
                argvvv[i] = (struct thargs *)malloc(sizeof(struct thargs));
                argvvv[i]->index = i;
                argvvv[i]->adrx = hpsdroptions.adrx[i];
                argvvv[i]->samplerate = (unsigned int)hpsdroptions.samplerate;
				argvvv[i]->alsabuffersize = (unsigned int)hpsdroptions.alsabuffersize;
                argvvv[i]->SamplesPerRx = SamplesPerRx;
                argvvv[i]->output_items = (short *) malloc (hpsdroptions.hermesbuffersize * sizeof(short) * SamplesPerRx * 2);
                memset(argvvv[i]->output_items, 0, hpsdroptions.hermesbuffersize * sizeof(short) * SamplesPerRx * 2);
                argvvv[i]->hermesbuffersize = hpsdroptions.hermesbuffersize;
                argvvv[i]->stopth = false;
                argvvv[i]->RxWriteCounter=0;
                argvvv[i]->state=1;
                pthread_create(&alsadeviceth[i],&attr,&alsapbth,(void*)argvvv[i]);
        }

        for(int i=1; i<=NumRx; ++i) {
                while(argvvv[i]->state==1) usleep(5);
                if(argvvv[i]->state == -1) {
                        Hermes->End();
                        for(int i=1; i<=NumRx; i++) {
                                argvvv[i]->stopth = 1;
                        }
                        exit(1);
                }
        }



        IQBuf_t Rx;
        unsigned RxWriteCounter=0;

        int waittotimeperframe = ((SamplesPerRx*1000000)/hpsdroptions.samplerate)/4;

        fprintf (stderr,"\033[0;34m");
        fprintf (stderr,"Press any key to exit\n");
        fprintf (stderr,"\033[0m");

        while(!kbhit()) {
                if( (Rx = Hermes->GetRxIQ()) != NULL) {
                        for (int index=0; index<SamplesPerRx; ++index)
                                for (int receiver=0; receiver < NumRx; ++receiver) {
                                        int indexotp = index*2+(RxWriteCounter* SamplesPerRx * 2);
                                        argvvv[receiver+1]->output_items[indexotp]= *Rx++;
                                        argvvv[receiver+1]->output_items[indexotp+1]= *Rx++;
                                }
                        for(int i=1; i<=NumRx; ++i) {
                                argvvv[i]->RxWriteCounter = RxWriteCounter;
                        }
                        ++RxWriteCounter; if(RxWriteCounter > hpsdroptions.hermesbuffersize) RxWriteCounter=0;
                }else{usleep(waittotimeperframe/10);}
        }
        Hermes->End();

        for(int i=1; i<=NumRx; ++i) {
                argvvv[i]->stopth = 1;
        }



        usleep(500);
        exit(0);

}
