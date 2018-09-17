ASYNC_SOURCES := $(shell find async -name '*.c')
ASYNC_TARGETS := $(basename $(notdir ${ASYNC_SOURCES}))

CFLAGS=-fopenmp -llightnvm -laio -lspdk_nvme -lspdk_util -lspdk_log -lspdk_env_dpdk -lrte_bus_pci -lrte_eal -lrte_mempool -lrte_mempool_ring -lrte_pci -lrte_ring -lrt -ldl -lnuma -luuid -std=c11 -Wall -Wextra -pedantic

default: ex01-completion-wait

all: $(ASYNC_TARGETS)

$(ASYNC_TARGETS): %: async/%.c
	@mkdir -p bin
	$(CC) $< -o bin/$@ ${CFLAGS}

clean:
	@mkdir -p bin
	rm -r bin
