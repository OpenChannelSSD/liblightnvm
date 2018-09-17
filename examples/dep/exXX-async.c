#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <liblightnvm.h>

#define NCHUNKS 4

static void write_cb(struct nvm_ret *ret, void *opaque)
{
	printf("write cb OK\n");
	free(ret);
}

int main(int argc, char **argv)
{
	int err;

	struct nvm_dev *dev = nvm_dev_openf("/dev/nvme0n1", NVM_BE_LBD);
	if (!dev) {
		perror("failed to open device");
		return EXIT_FAILURE;
	}
	const struct nvm_geo *geo = nvm_dev_get_geo(dev);

	struct nvm_addr slbas[NCHUNKS];
	if (0 != (err = nvm_cmd_rprt_arbs(dev, NVM_CHUNK_STATE_FREE, NCHUNKS, slbas))) {
		fprintf(stderr, "could not locate free chunks: %d\n", err);
		return EXIT_FAILURE;
	}

	const int ws_opt = nvm_dev_get_ws_opt(dev);

	struct nvm_buf_set *bufs = nvm_buf_set_alloc(dev, ws_opt * geo->sector_nbytes, 0);
	if (!bufs) {
		fprintf(stderr, "could not allocate buffer set: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}
	nvm_buf_set_fill(bufs);

	// setup async context
	struct nvm_async_ctx *ctx;
	if (NULL == (ctx = nvm_async_init(dev, 32))) {
		perror("could not initialize async context");
		return EXIT_FAILURE;
	}

	for (size_t sectr = 0; sectr < geo->l.nsectr; sectr += ws_opt) {
		// submit io to chunks in parallel
		for (size_t cnk = 0; cnk < NCHUNKS; cnk++) {
			struct nvm_addr addr = slbas[cnk];
			addr.l.sectr = sectr;

			struct nvm_ret *ret = malloc(sizeof(*ret));
			while (NULL == (ret->async = nvm_async_prep(dev, ctx, write_cb, NULL))) {
				if (errno == EAGAIN) {
					printf("EAGAIN; poking...\n");
					nvm_async_poke(dev, ctx);
					continue;
				}

				perror("unhandled error");
				return EXIT_FAILURE;
			}

			if (nvm_cmd_write(dev, &addr, ws_opt, bufs->write, NULL, NVM_CMD_SCALAR | NVM_CMD_ASYNC, ret)) {
				perror("nvm_cmd_write failed");
				return EXIT_FAILURE;
			}
		}
	}

	// reap all lingering requests
	nvm_async_wait(dev, ctx);
}
