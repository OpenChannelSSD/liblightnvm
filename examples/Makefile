SOURCES := $(shell find src -name '*.c')
TARGETS := $(basename $(notdir ${SOURCES}))

CFLAGS=-fopenmp -llightnvm_cli -llightnvm -laio -lspdk_nvme -lspdk_util -lspdk_log -lspdk_env_dpdk -lrte_bus_pci -lrte_eal -lrte_mempool -lrte_mempool_ring -lrte_pci -lrte_ring -lrt -ldl -lnuma -luuid -std=c11 -Wall -Wextra -pedantic

default: vblk-ex01-ewr.c

all: $(TARGETS)

$(TARGETS): %: src/%.c
	@mkdir -p bin
	$(CC) $< -o bin/$@ ${CFLAGS}

clean:
	@mkdir -p bin
	rm -r bin
