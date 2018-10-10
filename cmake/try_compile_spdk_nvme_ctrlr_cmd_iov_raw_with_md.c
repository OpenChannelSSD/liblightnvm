#include <spdk/nvme.h>

int main(int argc, char *argv[]) {
	spdk_nvme_ctrlr_cmd_iov_raw_with_md(NULL,
					    NULL,
					    NULL,
					    NULL,
					    NULL,
					    NULL,
					    NULL,
					    NULL);
	return 0;
}
