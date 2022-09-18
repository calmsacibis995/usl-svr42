#ident	"@(#)debugger:libexecon/common/Procctl.C	1.1"

#include "Procctl.h"
#include "Interface.h"
#include "utility.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

// Provides read access to static object files and core files.

int 
Procctl::open(int textfd)
{
	struct stat	stbuf;

	if ((fd = debug_dup(textfd)) == -1)
		return 0;	// no core file
	if (fstat(fd, &stbuf) == -1)
		return 0;
	ptype = pt_object;
	return 1;
}

int
Procctl::close()
{
	::close(fd);
	fd = -1;
	return 1;
}

int
Procctl::read(Iaddr from, void *to, int len)
{
	int	result;
	do {
		errno = 0;
		lseek(fd, from, SEEK_SET);
	} while(errno == EINTR);
	if (errno)
		return -1;
	do {
		errno = 0;
		result = ::read(fd, to, len);
	} while(errno == EINTR);
	return result;
}
