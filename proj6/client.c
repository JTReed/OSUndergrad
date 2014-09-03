#include <stdio.h>
#include <stdlib.h>
#include "mfs.h"

int main(int argc, char *argv[])
{
	MFS_Stat_t stat;

	MFS_Init(argv[1], atoi(argv[2]));
        MFS_Creat(0, MFS_REGULAR_FILE, "FOOBAR");
        MFS_Lookup(0, "FOOBAR");
        MFS_Stat(1, &stat);
        MFS_Write(1, "FOOBAR", 0);
        MFS_Read(1234, NULL, 1234);
        MFS_Unlink(0, "FOOBAR");

	return 0;
}

/*
 * Local variables:
 * c-basic-offset: 8
 * tab-width: 8
 * indent-tabs-mode: t
 * End:
 *
 * vi: set shiftwidth=8 tabstop=8 noexpandtab:
 * :indentSize=8:tabSize=8:noTabs=false:
 */
