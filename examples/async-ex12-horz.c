/**
 * Parallel IO through ASYNC CMD options
 *
 * The intent of this example is to:
 *
 * - Show the ASYNC infrastructure of liblightnvm
 * - Perform writes while respecting write-constraints, using WS_OPT, and
 *   contiguous access
 * - Increasing throughout via ASYNC and a horizontal striping scheme over
 *   parallel units
 *
 * Boiler-plate utilities is used for opening the device, retrieve a set of
 * disjoint chunks on distinct parallel units and erases are done in SYNC mode.
 *
 * The ASYNC part is provided in the functions `async_ex12_horz` and `horz`.
 * `async_ex12_horz` initializes and tears down the ASYNC CTX and invokes `horz`
 * drives the IO.
 *
 * The program terminates with 0 on success and EXIT_FAILURE otherwise.
 *
 * NOTE: it is not intended as a reference implementation for optimal
 * performance. You can use this as the baseline and experiment with ways of
 * sustaining queue-pressure, e.g. submit in the call-back on writes, use a
 * different submission-path for read as they are not bound by the contiguous
 * ordering constraint, reduce ret-val allocations, indexing / addressing
 * computations etc.
 */
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <liblightnvm.h>
#include <liblightnvm_cli.h>

/**
 * Here you process the completed IO, 'ret' provides the return status of the
 * command and 'cb_arg' can has whatever it was given.
 */
static void callback(struct nvm_ret *NVM_UNUSED(ret), void *NVM_UNUSED(cb_arg))
{
	// Do something useful upon completion. Use the definition below if you
	// want to debug it / see the callbacks happening
}

/*
static void callback(struct nvm_ret *ret, void *cb_arg)
{
	printf("# callback: ret: %p, cb_arg: %p,\n", (void*)ret, cb_arg);
}
*/

/**
 * Perform horizontal tiling with a maximum of 'nchunks' outstanding
 */
int horz(struct nvm_bp *bp, struct nvm_async_ctx *ctx, enum nvm_dio_opcodes opc)
{
	struct nvm_addr *chunk_addrs = bp->addrs;
	const size_t nchunks = bp->naddrs;
	const size_t tsectr = nchunks * bp->geo->l.nsectr;
	const size_t stripe_nsectr = bp->ws_opt;
	const size_t tstripe = tsectr / stripe_nsectr;
	struct nvm_ret rets[tstripe];

	int err;

	for (size_t stripe = 0; stripe < tstripe; ++stripe) {
		size_t cidx = stripe % nchunks;
		size_t c_ofz = (stripe / nchunks) * stripe_nsectr;
		size_t b_ofz = bp->geo->l.nbytes * stripe_nsectr * stripe;

		struct nvm_addr addrs[stripe_nsectr];
		struct nvm_ret *ret = &rets[stripe];

		for (size_t aidx = 0; aidx < stripe_nsectr; ++aidx ) {
			addrs[aidx].val = chunk_addrs[cidx].val;
			addrs[aidx].l.sectr = c_ofz + aidx;
		}

		// Setup pr-command ASYNC properties
		ret->async.ctx = ctx;		// Assign sub/cmpl context
		ret->async.cb = callback;	// Assign completion cb
		ret->async.cb_arg = NULL;	// Assign completion cb arg

		switch(opc) {
		case NVM_DOPC_VECTOR_WRITE:
		case NVM_DOPC_SCALAR_WRITE:
			err = nvm_cmd_write(bp->dev, addrs, stripe_nsectr,
					    bp->bufs->write + b_ofz, NULL,
					    NVM_CMD_ASYNC | NVM_CMD_VECTOR, ret);
			if (err) {
				perror("nvm_cmd_write");
				return -1;
			}
			break;

		case NVM_DOPC_SCALAR_READ:
		case NVM_DOPC_VECTOR_READ:
			err = nvm_cmd_read(bp->dev, addrs, stripe_nsectr,
					   bp->bufs->read + b_ofz, NULL,
					   NVM_CMD_ASYNC | NVM_CMD_VECTOR, ret);
			if (err) {
				perror("nvm_cmd_read");
				return -1;
			}
			break;

		default:
			break;
		}

		// SYNC after 'nchunk' submissions
		if (!((stripe + 1) % nchunks)) {
			if (nvm_async_wait(bp->dev, ctx) < 0) {
				perror("nvm_async_wait");
				return -1;
			}
		}
	}

	return 0;
}

int async_ex12_horz(struct nvm_bp *bp)
{
	struct nvm_async_ctx *ctx;
	int depth = 0;
	size_t diff;
	int err;

	// Initialize ASYNC CMD context
	ctx = nvm_async_init(bp->dev, depth, 0x0);
	if (!ctx) {
		perror("could not initialize async context");
		return -1;
	}

	// Erase SYNC
	err = nvm_cmd_erase(bp->dev, bp->addrs, bp->naddrs, NULL, 0x0, 0);
	if (err) {
		perror("nvm_cmd_erase");
		return -1;
	}

	// Write ASYNC
	printf("# nvm_cmd_write(NVM_CMD_ASYNC | VECTOR_WRITE)\n");
	nvm_cli_timer_start();

	err = horz(bp, ctx, NVM_DOPC_VECTOR_WRITE);
	if (err) {
		perror("horz(VECTOR_WRITE)");
		return -1;
	}

	nvm_cli_timer_stop();
	nvm_cli_timer_bw_pr("nvm_cmd_read", bp->bufs->nbytes);

	printf("# nvm_cmd_read(NVM_CMD_ASYNC | VECTOR_READ)\n");
	nvm_cli_timer_start();

	// Read ASYNC
	err = horz(bp, ctx, NVM_DOPC_VECTOR_READ);
	if (err) {
		perror("horz(VECTOR_READ)");
		return -1;
	}
	nvm_cli_timer_stop();
	nvm_cli_timer_bw_pr("nvm_cmd_read", bp->bufs->nbytes);

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

	err = async_ex12_horz(bp);
	if (err) {
		perror("async_ex12_horz");
		err = EXIT_FAILURE;
	}

	nvm_bp_term(bp);
	return err;
}

