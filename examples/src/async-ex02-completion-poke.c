/**
 * Example of using ASYNC CMD options
 *
 * This is the bare minimum for ASYNC CMD, the example does boiler-plate of the
 * below tasks using SYNC CMD:
 *
 * - Retrieve free chunk
 * - Erase the chunk
 * - Write a synthetic payload to the chunk
 *
 * The ASYNC part is provided in the function 'ex01_async_read', doing:
 *
 * - Initialize a CMD context
 * - Define an CMD callback function
 * - Setup 'nvm_ret' with callback function and callback arguments
 * - Submit reads for the entire chunk
 * - Poke for completion and overlap with something else
 * - Terminate the CMD context
 *
 * The program terminates with 0 on success and EXIT_FAILURE otherwise.
 */
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <liblightnvm.h>

#define SOMETHING_ELSE_USEC 10000

/**
 * One use of ASYNC IO is to have the CPU executing something useful instead of
 * it being blocked while waiting for completion of a given IO.
 *
 * This is mimicked in the following function which just sleeps 'usec'
 * microseconds
 *
 * Try running this with different values of the 'SOMETHING_ELSE_USEC' to see
 * the effect on completions per call to 'nvm_async_poke'
 */
static void do_something_else(int usec)
{
	struct timespec nap = { .tv_sec = 0, .tv_nsec = usec * 1000 };

	nanosleep(&nap, NULL);
}

/**
 * Here you process the completed IO, 'ret' provides the return status of the
 * command and 'cb_arg' can has whatever it was given. In this a pointer to the
 * 'outstanding' counter.
 */
static void callback(struct nvm_ret *ret, void *cb_arg)
{
	int *outstanding = cb_arg;

	*outstanding = *outstanding - 1;
	printf("# callback: ret: %p, outstanding: %d,\n", (void*)ret,
	       *outstanding);
}

int ex01_async_read(struct nvm_bp *bp)
{
	const uint32_t depth = bp->geo->l.nsectr / bp->ws_opt;
	struct nvm_ret rets[depth];
	struct nvm_async_ctx *ctx;
	int outstanding = 0;
	size_t diff;
	ssize_t res;

	// Initialize ASYNC CMD context
	ctx = nvm_async_init(bp->dev, depth, 0x0);
	if (!ctx) {
		perror("could not initialize async context");
		return -1;
	}

	printf("# nvm_vblk_erase\n");
	res = nvm_vblk_erase(bp->vblk);
	if (res < 0) {
		perror("nvm_vblk_erase");
		return -1;
	}

	printf("# nvm_vblk_write\n");
	res = nvm_vblk_write(bp->vblk, bp->bufs->write, bp->bufs->nbytes);
	if (res < 0) {
		perror("nvm_vblk_write");
		return -1;
	}

	// Submit read commands
	printf("# nvm_cmd_read - submit ...\n");
	for (size_t sectr = 0; sectr < bp->geo->l.nsectr; sectr += bp->ws_opt) {
		const size_t offset = sectr * bp->geo->l.nbytes;
		struct nvm_ret *ret = &rets[outstanding];
		struct nvm_addr addrs[bp->ws_opt];
		int err;

		for (size_t idx = 0; idx < bp->ws_opt; ++idx) {
			addrs[idx].val = bp->addrs[0].val;
			addrs[idx].l.sectr = sectr + idx;
		}

		// Setup pr-command ASYNC properties
		ret->async.ctx = ctx;			// Assign sub/cmpl context
		ret->async.cb = callback;		// Assign completion cb
		ret->async.cb_arg = &outstanding;	// Assign completion cb arg

		++outstanding;

		err = nvm_cmd_read(bp->dev, addrs, bp->ws_opt,
				   bp->bufs->read + offset, NULL,
				   NVM_CMD_VECTOR | NVM_CMD_ASYNC, ret);
		if (err) {
			--outstanding;
			perror("# nvm_cmd_write failed");
			return -1;
		}
	}

	// Process completions while doing something else
	printf("# nvm_cmd_read - outstanding: %d\n", outstanding);
	while (outstanding) {
		res = nvm_async_poke(bp->dev, ctx, 0);
		if (res < 0) {
			perror("nvm_async_poke");
			break;
		} else if (res) {
			printf("# nvm_async_poke: completed: %zd,"
			       " outstanding: %d\n", res, outstanding);
		}

		do_something_else(SOMETHING_ELSE_USEC);
	}

	// Tear down the ASYNC context
	printf("# nvm_async_term\n");
	if (nvm_async_term(bp->dev, ctx)) {
		perror("# nvm_async_term");
		return -1;
	}

	// Sanity check: did we actually read from device
	diff = nvm_buf_diff(bp->bufs->write, bp->bufs->read,
				   bp->bufs->nbytes);
	if (diff) {
		nvm_buf_diff_pr(bp->bufs->write, bp->bufs->read,
				bp->bufs->nbytes);
		errno = EIO;
		return -1;
	}

	return 0;
}

int main(int argc, char **argv)
{
	struct nvm_bp *bp;
	int err = EXIT_FAILURE;

	bp = nvm_bp_init_from_args(argc, argv);
	if (!bp) {
		perror("nvm_bp_init");
		return err;
	}

	err = ex01_async_read(bp);
	if (err) {
		perror("ex01_async_read");
		err = EXIT_FAILURE;
	}

	nvm_bp_term(bp);
	return err;
}

