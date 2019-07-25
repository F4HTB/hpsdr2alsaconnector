#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include "alsathreads.h"
#include <errno.h>

bool listdev(char *devname)

{

        bool result = false;
        char** hints;
        int err;
        char** n;
        char*  name;
        char*  desc;
        char*  ioid;

        /* Enumerate sound devices */
        err = snd_device_name_hint(-1, "pcm", (void***)&hints);
        if (err != 0) {

                fprintf(stderr, "*** Cannot get device names\n");
                exit(1);

        }

        n = hints;
        while (*n != NULL) {

                name = snd_device_name_get_hint(*n, "NAME");
                desc = snd_device_name_get_hint(*n, "DESC");
                ioid = snd_device_name_get_hint(*n, "IOID");

                if(strcmp(devname,name)==0) result = true;

                if (name && strcmp("null", name)) free(name);
                if (desc && strcmp("null", desc)) free(desc);
                if (ioid && strcmp("null", ioid)) free(ioid);
                n++;


        }

        //Free hint buffer too
        snd_device_name_free_hint((void**)hints);
        return result;

}


void* alsapbth(void *input)
{

        int index = ((struct thargs*)input)->index;
        char *adrx = ((struct thargs*)input)->adrx;
        unsigned int samplerate = ((struct thargs*)input)->samplerate;
        int SamplesPerRx = ((struct thargs*)input)->SamplesPerRx;
        unsigned RxWriteCounterth=((struct thargs*)input)->RxWriteCounter;
        fprintf (stdout,"new th %i on %s with samplerate %i\n", index, adrx,samplerate);

        int waittotimeperframe = ((SamplesPerRx*1000000)/samplerate)/4;

        int err;

        snd_pcm_t *playback_handle;
        snd_pcm_hw_params_t *hw_params;

        if(!listdev(adrx)) {
                fprintf (stderr,"\033[1;31m");
                fprintf (stderr, "rx%d %s no such device \n",index,adrx);
                fprintf (stderr,"\033[0m");
                ((struct thargs*)input)->state=-1;
                pthread_exit( NULL );
        }

        if ((err = snd_pcm_open (&playback_handle, adrx, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
                fprintf (stderr,"\033[1;31m");
                fprintf (stderr, "rx%d cannot open audio device %s (%s)\n",index,adrx,snd_strerror (err));
                fprintf (stderr,"\033[0m");
                ((struct thargs*)input)->state=-1;
                pthread_exit( NULL );
        }


        if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
                fprintf (stderr,"\033[1;31m");
                fprintf (stderr, "rx%d cannot allocate hardware parameter structure %s (%s)\n",index,adrx,snd_strerror (err));
                fprintf (stderr,"\033[0m");
                ((struct thargs*)input)->state=-1;
                pthread_exit( NULL );
        }


        if ((err = snd_pcm_hw_params_any (playback_handle, hw_params)) < 0) {
                fprintf (stderr,"\033[1;31m");
                fprintf (stderr, "rx%d cannot initialize hardware parameter structure %s (%s)\n",index,adrx,snd_strerror (err));
                fprintf (stderr,"\033[0m");
                ((struct thargs*)input)->state=-1;
                pthread_exit( NULL );
        }


        if ((err = snd_pcm_hw_params_set_access (playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
                fprintf (stderr,"\033[1;31m");
                fprintf (stderr, "rx%d cannot set access type %s (%s)\n",index,adrx,snd_strerror (err));
                fprintf (stderr,"\033[0m");
                ((struct thargs*)input)->state=-1;
                pthread_exit( NULL );
        }


        if ((err = snd_pcm_hw_params_set_format (playback_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
                fprintf (stderr,"\033[1;31m");
                fprintf (stderr, "rx%d cannot set sample format %s (%s)\n",index,adrx,snd_strerror (err));
                fprintf (stderr,"\033[0m");
                ((struct thargs*)input)->state=-1;
                pthread_exit( NULL );
        }


        if ((err = snd_pcm_hw_params_set_rate_near (playback_handle, hw_params, &samplerate, 0)) < 0) {
                fprintf (stderr,"\033[1;31m");
                fprintf (stderr, "rx%d cannot set sample rate %s (%s)\n",index,adrx,snd_strerror (err));
                fprintf (stderr,"\033[0m");
                ((struct thargs*)input)->state=-1;
                pthread_exit( NULL );
        }


        if ((err = snd_pcm_hw_params_set_channels (playback_handle, hw_params, 2)) < 0) {
                fprintf (stderr,"\033[1;31m");
                fprintf (stderr, "rx%d cannot set channel count %s (%s)\n",index,adrx,snd_strerror (err));
                fprintf (stderr,"\033[0m");
                ((struct thargs*)input)->state=-1;
                pthread_exit( NULL );
        }


        if ((err = snd_pcm_hw_params (playback_handle, hw_params)) < 0) {
                fprintf (stderr,"\033[1;31m");
                fprintf (stderr, "rx%d cannot set parameters %s (%s)\n",index,adrx,snd_strerror (err));
                fprintf (stderr,"\033[0m");
                ((struct thargs*)input)->state=-1;
                pthread_exit( NULL );
        }


        snd_pcm_hw_params_free (hw_params);

        if ((err = snd_pcm_prepare (playback_handle)) < 0) {
                fprintf (stderr,"\033[1;31m");
                fprintf (stderr, "rx%d cannot prepare audio interface for use %s (%s)\n",index,adrx,snd_strerror (err));
                fprintf (stderr,"\033[0m");
                ((struct thargs*)input)->state=-1;
                pthread_exit( NULL );
        }

        while((int)((struct thargs*)input)->RxWriteCounter < (int)(RxWriteCounterth+(((struct thargs*)input)->buffersize/2))) {
                usleep(waittotimeperframe);
                ((struct thargs*)input)->state=2;
        }


        fprintf (stdout,"start play th %i on %s with samplerate %i\n", index, adrx,samplerate);

        while(!((struct thargs*)input)->stopth) {
          if(((struct thargs*)input)->RxWriteCounter != RxWriteCounterth) {
                if ((err = snd_pcm_writei (playback_handle, ((struct thargs*)input)->output_items+(RxWriteCounterth * SamplesPerRx * 2), SamplesPerRx)) != SamplesPerRx) {
                        snd_pcm_recover(playback_handle, err, 0);
                        fprintf (stderr, "write to audio interface failed (%s)\n",
                                 snd_strerror (err));
                }

                        ++RxWriteCounterth; if(RxWriteCounterth > ((struct thargs*)input)->buffersize) RxWriteCounterth=0;
                }else{usleep(waittotimeperframe/10);}

        }

        snd_pcm_close (playback_handle);
        fprintf (stdout,"end of th %i on %s with samplerate %i\n", index, adrx,samplerate);
        pthread_exit( NULL );
}
