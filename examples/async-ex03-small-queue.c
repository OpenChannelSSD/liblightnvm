#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <liblightnvm.h>

#define IODEPTH 4

struct nvm_ret **rets;
uint32_t outstanding;

/**
 * Here you process the completed IO, 'ret' provides the return status of the
 * command and 'cb_arg' can has whatever it was given. In this a pointer to the
 * 'outstanding' counter.
 */
static void callback(struct nvm_ret *ret, void *cb_arg)
{
	struct nvm_ret **rets = cb_arg;
	memset(ret, 0, sizeof(*ret));
	rets[--outstanding] = ret;

	printf("# callback: ret: %p, cb_arg: %p,\n", (void*)ret, cb_arg);
}
static int read_vector(struct nvm_bp *bp, struct nvm_async_ctx *ctx, size_t sectr)
{
	if (outstanding == IODEPTH) {
		errno = EAGAIN;
		return -1;
	}

	const size_t offset = sectr * bp->geo->l.nbytes;
	struct nvm_ret *ret = rets[outstanding++];
	struct nvm_addr addrs[bp->ws_opt];

	for (size_t idx = 0; idx < bp->ws_opt; ++idx) {
		addrs[idx].val = bp->addrs[0].val;
		addrs[idx].l.sectr = sectr + idx;
	}

	// Setup pr-command ASYNC properties
	ret->async.ctx = ctx;		// Assign sub/cmpl context
	ret->async.cb = callback;	// Assign completion cb
	ret->async.cb_arg = rets;	// Assign completion cb arg

	return nvm_cmd_read(bp->dev, addrs, bp->ws_opt,
			    bp->bufs->read + offset, NULL,
			    NVM_CMD_VECTOR | NVM_CMD_ASYNC, ret);
}

int ex03_small_queue(struct nvm_bp *bp)
{
	rets = calloc(IODEPTH, sizeof(struct nvm_ret *));
	for (uint32_t i = 0; i < IODEPTH; i++)
		rets[i] = calloc(1, sizeof(struct nvm_ret));

	struct nvm_async_ctx *ctx;
	size_t diff;
	ssize_t res;

	// Initialize ASYNC CMD context
	ctx = nvm_async_init(bp->dev, IODEPTH, 0x0);
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
		while(0 != read_vector(bp, ctx, sectr)) {
			if (errno == EAGAIN) {
				int nevents = nvm_async_poke(bp->dev, ctx, 0);
				if (nevents < 0) {
					perror("# nvm_async_poke");
					goto exit;
				}

				continue;
			}

			perror("# nvm_cmd_read");
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

	err = ex03_small_queue(bp);
	if (err) {
		perror("ex01_async_read");
		err = EXIT_FAILURE;
	}

	nvm_bp_term(bp);
	return err;
}
