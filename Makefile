BUILD_TYPE?=Release
BUILD_DIR?=build
BUILD_TESTS?=ON
BUILD_EXAMPLES?=ON

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
	cd $(BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DTESTS=$(BUILD_TESTS) -DEXAMPLES=$(BUILD_EXAMPLES) ../
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
test_dev:
	sudo nvm_test_dev nvme0n1

test_vblock:
	sudo nvm_test_vblock nvme0n1

test_vblock_rw:
	sudo nvm_test_vblock_rw nvme0n1

test_vblock_gp_n:
	sudo nvm_test_vblock_gp_n nvme0n1

test_concur:
	sudo nvm_test_concur nvme0n1

test_beam:
	sudo nvm_test_beam nvme0n1

# ... all of them
test: test_dev test_vblock test_vblock_rw test_vblock_gp_n test_concur test_beam

test_nb: test_dev test_vblock test_vblock_rw test_vblock_gp_n test_concur

# Invoking examples ...
ex_info:
	@sudo nvm_ex_info nvme0n1 || true

ex_vblock_pio_1:
	@sudo nvm_ex_vblock_pio_1 nvme0n1 || true

ex_vblock_pio_n:
	@sudo nvm_ex_vblock_pio_n nvme0n1 || true

example: ex_info ex_vblock_pio_1 ex_vblock_pio_n

# ... all of them

# Removes everything, build and install package
dev: pkg_uninstall clean configure make pkg pkg_install
