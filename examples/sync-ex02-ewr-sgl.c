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

int sync_ex02_ewr_sgl(struct nvm_bp *bp)
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

	printf("# nvm_cmd_write\n");
	nvm_cli_timer_start();
	for (size_t cidx = 0; cidx < nchunks; ++cidx) {
		const size_t ofz = cidx * bp->geo->l.nsectr * bp->geo->l.nbytes;

		for (size_t sectr = 0; sectr < bp->geo->l.nsectr; sectr += ws_opt) {
			const size_t buf_ofz = sectr * bp->geo->l.nbytes + ofz;
			struct nvm_sgl *sgl = nvm_sgl_create(bp->dev, 1);
			struct nvm_addr addrs[ws_opt];

			for (size_t aidx = 0; aidx < ws_opt; ++aidx) {
				const size_t buf_ofz_seg = buf_ofz + (aidx * bp->geo->l.nbytes);
				addrs[aidx].val = chunk_addrs[cidx].val;
				addrs[aidx].l.sectr = sectr + aidx;

				nvm_sgl_add(bp->dev, sgl, bp->bufs->write + buf_ofz_seg,
					    bp->geo->l.nbytes);
			}

			err = nvm_cmd_write(bp->dev, addrs, ws_opt, sgl, NULL,
					    NVM_CMD_SGL, NULL);
			if (err) {
				perror("nvm_cmd_write");
				return -1;
			}

			nvm_sgl_destroy(bp->dev, sgl);
		}
	}
	nvm_cli_timer_stop();
	nvm_cli_timer_bw_pr("nvm_cmd_write", bp->bufs->nbytes);

	printf("# nvm_cmd_read\n");
	nvm_cli_timer_start();
	for (size_t cidx = 0; cidx < nchunks; ++cidx) {
		const size_t ofz = cidx * bp->geo->l.nsectr * bp->geo->l.nbytes;

		for (size_t sectr = 0; sectr < bp->geo->l.nsectr; sectr += ws_opt) {
			const size_t buf_ofz = sectr * bp->geo->l.nbytes + ofz;
			struct nvm_sgl *sgl = nvm_sgl_create(bp->dev, 1);
			struct nvm_addr addrs[ws_opt];

			for (size_t aidx = 0; aidx < ws_opt; ++aidx) {
				size_t buf_ofz_seg = buf_ofz + (aidx * bp->geo->l.nbytes);
				addrs[aidx].val = chunk_addrs[cidx].val;
				addrs[aidx].l.sectr = sectr + aidx;

				nvm_sgl_add(bp->dev, sgl, bp->bufs->read + buf_ofz_seg,
					    bp->geo->l.nbytes);
			}

			err = nvm_cmd_read(bp->dev, addrs, ws_opt, sgl, NULL,
					    NVM_CMD_SGL, NULL);
			if (err) {
				perror("nvm_cmd_write");
				return -1;
			}

			nvm_sgl_destroy(bp->dev, sgl);
		}
	}
	nvm_cli_timer_stop();
	nvm_cli_timer_bw_pr("nvm_cmd_read", bp->bufs->nbytes);

	printf("# nvm_cmd_erase\n");
	err = nvm_cmd_erase(bp->dev, chunk_addrs, nchunks, NULL, 0x0, NULL);
	if (err) {
		perror("nvm_cmd_erase");
		return -1;
	}

	// Sanity check: did we read what we wrote?
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

	err = sync_ex02_ewr_sgl(bp);
	if (err) {
		perror("sync_ex02_ewr_sgl");
		err = EXIT_FAILURE;
	}

	nvm_bp_term(bp);
	return err;
}
