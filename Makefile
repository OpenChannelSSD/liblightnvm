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

tests_off:
	$(eval BUILD_TESTS := OFF)

examples_off:
	$(eval BUILD_EXAMPLES := OFF)

cmake_check:
	@cmake --version || (echo "\n** Please install 'cmake' **\n" && exit 1)

configure: cmake_check
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DTESTS=$(BUILD_TESTS) -DEXAMPLES=$(BUILD_EXAMPLES) ../
	@echo "Modify build configuration in '$(BUILD_DIR)'"

make:
	cd $(BUILD_DIR) && make

docs:
	@mkdir -p $(BUILD_DIR)/docs
	doxygen ci/doxy/docs.cfg

docs-view:
	xdg-open $(BUILD_DIR)/docs/html/index.html

docs-publish:
	rm -fr $(BUILD_DIR)/ghpages
	mkdir -p $(BUILD_DIR)/ghpages
	git clone -b gh-pages git@github.com:OpenChannelSSD/liblightnvm.git --single-branch $(BUILD_DIR)/ghpages
	cd $(BUILD_DIR)/ghpages && git rm -rf .
	cp -r $(BUILD_DIR)/docs/html/. $(BUILD_DIR)/ghpages/
	cd $(BUILD_DIR)/ghpages && git add .
	if [ -z "`git config user.name`" ]; then git config user.name "Mr. Robot"; fi
	if [ -z "`git config user.email`" ]; then git config user.email "foo@example.com"; fi
	cd $(BUILD_DIR)/ghpages && git commit -m "Autogen docs for `git rev-parse --short HEAD`."
	cd $(BUILD_DIR)/ghpages && git push origin gh-pages

install:
	cd $(BUILD_DIR) && make install

clean:
	rm -fr $(BUILD_DIR) || true
	rm -f tags || true

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
	ctags * -R .
	cscope -b `find . -name '*.c'` `find . -name '*.h'`

# Invoking tests ...
test_dev:
	sudo nvm_test_dev /dev/nvme0n1

test_mbad:
	sudo nvm_test_mbad /dev/nvme0n1

test_vblk:
	sudo nvm_test_vblk /dev/nvme0n1

test_vblk_gp_n:
	sudo nvm_test_vblk_gp_n /dev/nvme0n1

test_concur:
	sudo nvm_test_concur /dev/nvme0n1

# ... all of them
test: test_dev test_vblk test_vblk_gp_n test_mbad test_concur

# Invoking examples ...
ex_info:
	@sudo nvm_ex_info /dev/nvme0n1 || true

ex_vblock_pio_1:
	@sudo nvm_ex_vblock_pio_1 /dev/nvme0n1 || true

ex_vblock_pio_n:
	@sudo nvm_ex_vblock_pio_n /dev/nvme0n1 || true

example: ex_info ex_vblock_pio_1 ex_vblock_pio_n

# ... all of them

# Removes everything, build and install package
dev: pkg_uninstall clean configure make pkg pkg_install
