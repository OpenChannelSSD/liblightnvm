#include <stdio.h>
#include <liblightnvm.h>

int bounds_check(NVM_GEO geo, NVM_ADDR addrs[], int naddrs)
{
	int ninvalid = 0, i;
	int invalid = 0x0;

	for (i = 0; i < naddrs; ++i) {			// Check `addr`
		ninvalid = invalid_addr ? ninvalid + 1 : ninvalid;
	}

	return ninvalid;
}

