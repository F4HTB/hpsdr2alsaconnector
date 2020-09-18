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
	unsigned hermesbuffersize=4096;
	unsigned int alsabuffersize=128*1024;
};

#endif
