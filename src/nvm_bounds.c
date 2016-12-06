#include <liblightnvm.h>
#include <stdio.h>

/**
 * Prints a humanly readable description of given mask of bounds
 */
void nvm_bounds_pr(int mask)
{
	if (!mask) {
		printf("No bounds\n");
		return;
	}

	if (mask & NVM_BOUNDS_CHANNEL)
		printf("Channel bound\n");
	if (mask & NVM_BOUNDS_LUN)
		printf("LUN bound\n");
	if (mask & NVM_BOUNDS_PLANE)
		printf("Plane bound\n");
	if (mask & NVM_BOUNDS_BLOCK)
		printf("Block bound\n");
	if (mask & NVM_BOUNDS_PAGE)
		printf("Page bound\n");
	if (mask & NVM_BOUNDS_SECTOR)
		printf("Sector bound\n");
}

