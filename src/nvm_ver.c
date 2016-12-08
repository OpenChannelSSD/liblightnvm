#include <stdlib.h>
#include <string.h>

int nvm_ver_major(void)
{
	return 0;
}

int nvm_ver_minor(void)
{
	return 0;
}

int nvm_ver_patch(void)
{
	return 1;
}

void nvm_ver_pr(void)
{
	printf("ver { major(%d), minor(%d), patch(%d) }",
	       nvm_ver_major(), nvm_ver_minor(), nvm_ver_patch());
}

