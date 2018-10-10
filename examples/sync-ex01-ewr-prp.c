/**
 * Example of using using nvm_cmd for IO
 *
 *
 * The program terminates with 0 on success and EXIT_FAILURE otherwise.
 */
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <liblightnvm.h>
#include <liblightnvm_spec.h>
#include <liblightnvm_cli.h>

int sync_ex01_ewr_prp(struct nvm_bp *bp)
{
	const size_t nchunks = bp->naddrs;
	struct nvm_addr chunk_addrs[nchunks];
	size_t ws_opt = nvm_dev_get_ws_opt(bp->dev);
	size_t diff;
	int err;

	printf("# nvm_cmd EWR example 'nchunks: %zu' addresses\n", nchunks);
	err = nvm_cmd_rprt_arbs(bp->dev, NVM_CHUNK_STATE_FREE, nchunks,
				chunk_addrs);
	if (err) {
		perror("nvm_cmd_rprt_arbs");
		return -1;
	}

	printf("# nvm_cmd_erase\n");
	err = nvm_cmd_erase(bp->dev, chunk_addrs, nchunks, NULL, 0x0, NULL);
	if (err) {
		perror("nvm_cmd_erase");
		return -1;
	}

	printf("# nvm_cmd_write\n");
	nvm_cli_timer_start();
	for (size_t cidx = 0; cidx < nchunks; ++cidx) {
		const size_t ofz = cidx * bp->geo->l.nsectr * bp->geo->l.nbytes;

		for (size_t sectr = 0; sectr < bp->geo->l.nsectr; sectr += ws_opt) {
			size_t buf_ofz = sectr * bp->geo->l.nbytes + ofz;
			struct nvm_addr addrs[ws_opt];

			for (size_t aidx = 0; aidx < ws_opt; ++aidx) {
				addrs[aidx].val = chunk_addrs[cidx].val;
				addrs[aidx].l.sectr = sectr + aidx;
			}

			err = nvm_cmd_write(bp->dev, addrs, ws_opt,
					    bp->bufs->write + buf_ofz, NULL,
					    0x0, NULL);
			if (err) {
				perror("nvm_cmd_write");
				return -1;
			}
		}
	}
	nvm_cli_timer_stop();
	nvm_cli_timer_bw_pr("nvm_cmd_write", bp->bufs->nbytes);

	printf("# nvm_cmd_read\n");
	nvm_cli_timer_start();
	for (size_t cidx = 0; cidx < nchunks; ++cidx) {
		const size_t ofz = cidx * bp->geo->l.nsectr * bp->geo->l.nbytes;

		for (size_t sectr = 0; sectr < bp->geo->l.nsectr; sectr += ws_opt) {
			size_t buf_ofz = sectr * bp->geo->l.nbytes + ofz;
			struct nvm_addr addrs[ws_opt];

			for (size_t aidx = 0; aidx < ws_opt; ++aidx) {
				addrs[aidx].val = chunk_addrs[cidx].val;
				addrs[aidx].l.sectr = sectr + aidx;
			}

			err = nvm_cmd_read(bp->dev, addrs, ws_opt,
					   bp->bufs->read + buf_ofz, NULL,
					   0x0, NULL);
			if (err) {
				perror("nvm_cmd_read");
				return -1;
			}
		}
	}
	nvm_cli_timer_stop();
	nvm_cli_timer_bw_pr("nvm_cmd_read", bp->bufs->nbytes);

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

	err = sync_ex01_ewr_prp(bp);
	if (err) {
		perror("sync_ex01_ewr_prp");
		err = EXIT_FAILURE;
	}

	nvm_bp_term(bp);
	return err;
}

