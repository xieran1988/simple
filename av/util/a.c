
#include <sys/time.h>
#include <unistd.h>

float tm_elapsed()
{
	static struct timeval tv, last;

	gettimeofday(&tv, NULL);
	if (!last.tv_sec && !last.tv_usec) {
		last = tv;
		return 0;
	}
	float r = tv.tv_sec-last.tv_sec+(tv.tv_usec-last.tv_usec)/1e6;
	return r;
}

void fsleep(float tm)
{
	usleep(tm*1e6);
}

