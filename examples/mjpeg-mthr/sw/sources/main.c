#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <pthread.h>
#include <dpn.h>

#include "mjpeg.h"

extern void * fetch_process (void * c);
extern void * idct_process (void * c);
extern void * dispatch_process (void * c);

int main (void) {
	Channel * channel[2 * NB_IDCT + 1];
	Channel * fetch_channel[1 + NB_IDCT];
	Channel * dispatch_channel[1 + NB_IDCT];
	Channel * idct_channel[NB_IDCT][2];

	pthread_t fetchThread, dispatchThread, idctThread[NB_IDCT];
	pthread_attr_t fetchAttr, dispatchAttr, idctAttr[NB_IDCT];

       char buffer[128];

	channel[0] = channelInit ("/devices/rdv.0", 1);

	for (uint32_t i = 0; i < 2 * NB_IDCT; i++) {
	  sprintf(buffer,"/devices/rdv.%lu", i + 1);
		channel[i + 1] = channelInit (buffer, 1);
	}
	
	fetch_channel[0] = channel[0];
	dispatch_channel[0] = channel[0];

	for (uint32_t i = 0; i < NB_IDCT; i++) {
		fetch_channel[i + 1] = channel[2 * i + 1];
		dispatch_channel[i + 1] = channel[2 * i + 2];

		idct_channel[i][0] = channel[2 * i + 1];
		idct_channel[i][1] = channel[2 * i + 2];
	}

	printf ("[MJPEG] %d IDCT\n", NB_IDCT);

	for (uint32_t i = 0; i < NB_IDCT; i++) {
		pthread_attr_init (& idctAttr[i]);
		// idctAttr[i] . procid = i;
  	//	pthread_create (& idctThread[i], & idctAttr[i], idct_process, idct_channel[i]);
	  sprintf(buffer,"idct.%lu", i + 1);
		idctAttr[i] . name = buffer;
  	pthread_create (& idctThread[i], & idctAttr[i], idct_process, idct_channel[i]);
	}

	pthread_attr_init (& fetchAttr);
	//	fetchAttr . procid = 0;
	fetchAttr . name = "fetch";

	pthread_attr_init (& dispatchAttr);
	//	dispatchAttr . procid = 1;
	dispatchAttr . name = "dispatch";

  pthread_create (& fetchThread, & fetchAttr, fetch_process, fetch_channel);
  pthread_create (& dispatchThread, & dispatchAttr, dispatch_process, dispatch_channel);

  pthread_join(dispatchThread, NULL);
	return 0;
}
