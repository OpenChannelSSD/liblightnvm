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
 * - Wait for completion
 * - Terminate the CMD context
 *
 * The program terminates with 0 on success and EXIT_FAILURE otherwise.
 */
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <liblightnvm.h>

/**
 * Here you process the completed IO, 'ret' provides the return status of the
 * command and 'cb_arg' can has whatever it was given. In this a pointer to the
 * 'outstanding' counter.
 */
static void callback(struct nvm_ret *ret, void *cb_arg)
{
	printf("# callback: ret: %p, cb_arg: %p,\n", (void*)ret, cb_arg);
}

int ex01_completion_wait(struct nvm_bp *bp)
{
	const uint32_t depth = bp->geo->l.nsectr / bp->ws_opt;
	struct nvm_ret rets[depth];
	int count = 0;
	struct nvm_async_ctx *ctx;
	size_t diff;
	ssize_t res;

	// Initialize ASYNC CMD context
	ctx = nvm_async_init(bp->dev, depth, 0x0);
	if (!ctx) {
		perror("could not initialize async context");
		return -1;
	}

	printf("# nvm_vblk_write\n");
	res = nvm_vblk_write(bp->vblk, bp->bufs->write, bp->bufs->nbytes);
	if (res < 0) {
		perror("nvm_vblk_write");
		goto exit;
	}

	// Submit read commands
	printf("# nvm_cmd_read - submit ...\n");
	for (size_t sectr = 0; sectr < bp->geo->l.nsectr; sectr += bp->ws_opt) {
		const size_t offset = sectr * bp->geo->l.nbytes;
		struct nvm_ret *ret = &rets[count++];
		struct nvm_addr addrs[bp->ws_opt];
		int err;

		for (size_t idx = 0; idx < bp->ws_opt; ++idx) {
			addrs[idx].val = bp->addrs[0].val;
			addrs[idx].l.sectr = sectr + idx;
		}

		// Setup pr-command ASYNC properties
		ret->async.ctx = ctx;		// Assign sub/cmpl context
		ret->async.cb = callback;	// Assign completion cb
		ret->async.cb_arg = NULL;	// Assign completion cb arg

		err = nvm_cmd_read(bp->dev, addrs, bp->ws_opt,
				   bp->bufs->read + offset, NULL,
				   NVM_CMD_VECTOR | NVM_CMD_ASYNC, ret);
		if (err) {
			perror("# nvm_cmd_write failed");
			goto exit;
		}
	}

	// Wait for IO completion
	printf("# nvm_async_wait -- enter\n");
	res = nvm_async_wait(bp->dev, ctx);
	if (res < 0) {
		perror("nvm_async_wait");
		goto exit;
	}

	printf("# nvm_async_wait -- done, res: %zd\n", res);

	// Sanity check: did we actually read from device
	diff = nvm_buf_diff(bp->bufs->write, bp->bufs->read,
				   bp->bufs->nbytes);
	if (diff) {
		nvm_buf_diff_pr(bp->bufs->write, bp->bufs->read,
				bp->bufs->nbytes);
		errno = EIO;
		return -1;
	}

exit:
	// Tear down the ASYNC context
	printf("# nvm_async_term\n");
	if (nvm_async_term(bp->dev, ctx)) {
		perror("# nvm_async_term");
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

	err = ex01_completion_wait(bp);
	if (err) {
		perror("ex01_async_read");
		err = EXIT_FAILURE;
	}

	nvm_bp_term(bp);
	return err;
}
