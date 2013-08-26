/*
 * test.cpp
 *
 *  Created on: Aug 20, 2013
 *      Author: awahl
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <aio.h>
#include <signal.h>
#include "asynclib.h"
#include "asynclib.cpp"

typedef AsyncObject<int, int> UnixIOObj;

class AioObject : public UnixIOObj {
	int offset;
public:
	struct aiocb aioCb;
};

void aio_completion_handler( int signo, siginfo_t *info, void *context )
{
	struct aiocb *req;

	int ret;


	/* Ensure it's our signal */
	if (info->si_signo == SIGIO) {

		req = (struct aiocb *)info->si_value.sival_ptr;

		/* Did the request complete? */
		if (aio_error( req ) == 0) {

			/* Request completed successfully, get the return status */
			ret = aio_return( req );
		}
	}
	return;
}


void setupsigaction()
{
	struct sigaction sig_act;

	/* Set up the signal handler */
	sigemptyset(&sig_act.sa_mask);
	sig_act.sa_flags = SA_SIGINFO;
	sig_act.sa_sigaction = aio_completion_handler;

}


int asyncRead(UnixIOObj *obj, bool completed, int event)
{
	if (!completed)
	{
		AioObject *aioObj = static_cast<AioObject *>(obj);
		/* Set up the AIO request */
		memset(&aioObj->aioCb, 0, sizeof(struct aiocb));
		aioObj->aioCb.aio_fildes = aioObj->getDev();
		aioObj->aioCb.aio_buf = malloc(1024);
		aioObj->aioCb.aio_nbytes = 1023;
		aioObj->aioCb.aio_offset = next_offset;

		/* Link the AIO request with the Signal Handler */
		aioObj->aioCb.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
		aioObj->aioCb.aio_sigevent.sigev_signo = SIGIO;
		aioObj->aioCb.aio_sigevent.sigev_value.sival_ptr = &aioObj->aioCb;

		return aio_read( &aioObj->aioCb);
	}
	return 0;
}


int main(int argc, char *argv[])
{
	struct sigaction sig_act;
	struct aiocb my_aiocb;

	/* Set up the signal handler */
	sigemptyset(&sig_act.sa_mask);
	sig_act.sa_flags = SA_SIGINFO;
	sig_act.sa_sigaction = aio_completion_handler;

	int ret = sigaction( SIGIO, &sig_act, NULL );
	int fd = open("filename.txt", O_RDONLY);



	return 0;
}

