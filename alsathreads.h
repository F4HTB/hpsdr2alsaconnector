#ifndef ALSATHREADS_H
#define ALSATHREADS_H

struct thargs {
				short state = 0;
				unsigned RxWriteCounter=0;
				bool stopth = false;
				int SamplesPerRx;
        int index;
        char *adrx;
				unsigned int samplerate;
				short *output_items;
				unsigned buffersize=4096;
};

#endif
