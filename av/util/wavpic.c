#include "a.h"
#include <av/encdec/a.h>

int main()
{
	mp4dec_loglevel(1);

	void *dec = mp4dec_open("/vid/1.mp3");

	return 0;
}

