BUILD_TYPE?=Release
BUILD_DIR?=build

#
# Traditional build commands
#

default: configure make

debug:
	$(eval BUILD_TYPE := Debug)

cmake_check:
	@cmake --version || (echo "\n** Please install 'cmake' **\n" && exit 1)

configure: cmake_check
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) ../
	@echo "Modify build configuration in '$(BUILD_DIR)'"

make:
	cd $(BUILD_DIR) && make

install:
	cd $(BUILD_DIR) && make install

clean:
	rm -r $(BUILD_DIR) || true
	rm tags || true

all: clean default install

#
# Packages (currently debian/.deb)
#
pkg:
	cd $(BUILD_DIR) && make package

pkg_install:
	sudo dpkg -i $(BUILD_DIR)/*.deb

pkg_uninstall:
	sudo apt-get --yes remove liblightnvm || true

#
# Commands useful for development
#
#
tags:
	ctags * -R
	cscope -b `find . -name '*.c'` `find . -name '*.h'`

# Invoking tests ...
test_tgt_create:
	@sudo lnvm create -d nvme0n1 -n nvm_tst_tgt -t dflash || true

test_tgt_remove:
	@sudo lnvm remove -n nvm_tst_tgt || true

test_concur:
	@make test_tgt_create || true
	sudo nvm_test_concur nvm_tst_tgt dflash
	@make test_tgt_remove || true

test_mgmt:
	sudo nvm_test_mgmt nvme0n1 nvm_tst_tgt dflash

test_dev:
	sudo nvm_test_dev

test_vblock:
	sudo nvm_test_vblock nvme0n1

test_vblock_rw:
	sudo nvm_test_vblock_rw nvme0n1

test_beam:
	sudo nvm_test_beam nvme0n1

# ... all of them
test: test_mgmt test_dev test_tgt test_vblock test_vblock_rw test_beam

# Invoking examples ...

ex_tgt_create:
	@sudo lnvm create -d nvme0n1 -n nvm_ex_tgt -t dflash || true

ex_tgt_remove:
	@sudo lnvm remove -n nvm_ex_tgt || true

ex_nvmfs:
	@make ex_tgt_create || true
	@sudo mkdir -p /tmp/nvmfs:w || true
	@sudo umount /tmp/nvmfs || true
	@mkdir /tmp/nvmfs || true
	@sudo nvmfs /tmp/nvmfs || true
	@sudo ls -lah /tmp/nvmfs || true
	@sudo sh -c "cat /tmp/nvmfs/hello" || true
	@sudo sh -c "touch /tmp/nvmfs/file0" || true
	@sudo umount /tmp/nvmfs || true
	@make ex_tgt_remove || true

ex_vblock:
	@make ex_tgt_create
	@sudo nvm_ex_vblock nvm_ex_tgt || true
	@make ex_tgt_remove

ex_vblock_reserve_n:
	@make ex_tgt_create
	@sudo nvm_ex_vblock_reserve_n nvme0n1 nvm_ex_tgt 8 || true
	@make ex_tgt_remove

ex_vblock_rw:
	@make ex_tgt_create
	@sudo nvm_ex_vblock_rw nvme0n1 nvm_ex_tgt || true
	@make ex_tgt_remove

ex_vblock_wr:
	@make ex_tgt_create
	@sudo nvm_ex_vblock_wr nvme0n1 nvm_ex_tgt || true
	@make ex_tgt_remove

ex_vblock_rw_multilun:
	@make ex_tgt_create
	sudo nvm_ex_vblock_rw_multilun nvme0n1 nvm_ex_tgt || true
	@make ex_tgt_remove

ex_vblock_rw_all:
	@make ex_tgt_create
	sudo nvm_ex_vblock_rw_all nvme0n1 nvm_ex_tgt || true
	@make ex_tgt_remove

example: ex_vblock ex_vblock_reserve_n ex_vblock_rw ex_vblock_rw_all 

# ... all of them

# Removes everything, build and install package
dev: pkg_uninstall clean configure make pkg pkg_install
