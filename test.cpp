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
	AioObject(int d, const char *name, AsyncManager<int, int> *m) : UnixIOObj(d, name, m) {};
	struct aiocb aioCb;
};

#define IO_FINISHED 1

AsyncManager<int, int> mgr;

void aio_completion_handler( int signo, siginfo_t *info, void *context )
{
	struct aiocb *req;

	/* Ensure it's our signal */
	if (info->si_signo == SIGIO) {

		req = (struct aiocb *)info->si_value.sival_ptr;
		mgr.processEvent(req->aio_fildes, IO_FINISHED);

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
	AioObject *aioObj = static_cast<AioObject *>(obj);
	if (!completed)
	{
		/* Set up the AIO request */
		memset(&aioObj->aioCb, 0, sizeof(struct aiocb));
		aioObj->aioCb.aio_fildes = aioObj->getDev();
		if (!aioObj->aioCb.aio_buf)
			aioObj->aioCb.aio_buf = malloc(1024);
		aioObj->aioCb.aio_nbytes = 1023;
		aioObj->aioCb.aio_offset = aioObj->aioCb.aio_offset + 1023;

		/* Link the AIO request with the Signal Handler */
		aioObj->aioCb.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
		aioObj->aioCb.aio_sigevent.sigev_signo = SIGIO;
		aioObj->aioCb.aio_sigevent.sigev_value.sival_ptr = &aioObj->aioCb;

		return aio_read( &aioObj->aioCb);
	}
	/* Did the request complete? */
	if (aio_error( &aioObj->aioCb ) == 0) {
		int ret = aio_return( &aioObj->aioCb );

		/* Request completed successfully, get the return status */
		return ret;
	}
	return 0;
}


int main(int argc, char *argv[])
{
	struct sigaction sig_act;
	sigset_t mask;
	struct timespec timeout;
	siginfo_t sigInfo;
	int ret;

	sigemptyset(&mask);
	sigaddset(&mask, SIGIO);
	sigprocmask(SIG_BLOCK, &mask, NULL);

	/* Set up the signal handler */
	sigemptyset(&sig_act.sa_mask);
	sig_act.sa_flags = SA_SIGINFO;
	sig_act.sa_sigaction = aio_completion_handler;

	ret = sigaction( SIGIO, &sig_act, NULL );
	int fd = open("filename.txt", O_RDONLY);

	AioObject fobj(fd, "firstfile", &mgr);

	mgr.addAsyncObject(&fobj);

	AsyncFunction<int, int> aioRead("asyncRead", asyncRead, IO_FINISHED);


	std::cerr << "Is SyncFunction=" << aioRead.isSyncFunction() << std::endl;


	fobj << &aioRead;

	timeout.tv_nsec = 0;
	timeout.tv_sec = 1;

	fobj.launchFunctions();
	while(true)
	{
		ret = sigtimedwait(&mask, &sigInfo, &timeout);
		if (ret == -1)
		{
			std::cerr << "some sigerror=" << errno << std::endl;
		}
		else
			aio_completion_handler(ret, &sigInfo, NULL);

		sleep(5);
	}


	return 0;
}

