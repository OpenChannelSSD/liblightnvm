BUILD_TYPE?=Release
BUILD_DIR?=build
BUILD_TESTS?=OFF
BUILD_EXAMPLES?=OFF

#
# Traditional build commands / make interface
#
default: make

.PHONY: debug
debug:
	$(eval BUILD_TYPE := Debug)

.PHONY: tests
tests:
	$(eval BUILD_TESTS := ON)

.PHONY: examples
examples:
	$(eval BUILD_EXAMPLES := ON)

.PHONY: cmake_check
cmake_check:
	@cmake --version || (echo "\n** Please install 'cmake' **\n" && exit 1)

.PHONY: configure
configure: cmake_check
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DTESTS=$(BUILD_TESTS) -DEXAMPLES=$(BUILD_EXAMPLES) ../
	@echo "Modify build configuration in '$(BUILD_DIR)'"

.PHONY: make
make: configure
	cd $(BUILD_DIR) && make

.PHONY: install
install:
	cd $(BUILD_DIR) && make install

.PHONY: make-pkg
make-pkg: configure
	cd $(BUILD_DIR) && make package

.PHONY: install-pkg
install-pkg:
	sudo dpkg -i $(BUILD_DIR)/*.deb

.PHONY: uninstall-pkg
uninstall-pkg:
	sudo apt-get --yes remove liblightnvm || true

.PHONY: clean
clean:
	rm -fr $(BUILD_DIR) || true
	rm -f tags || true

all: clean default install

# Removes packages, cleans up, builds lib, examples, tests, pkg and installs
.PHONY: dev
dev: uninstall-pkg clean examples tests make-pkg install-pkg

#
# Experimental section
#
.PHONY: docs
docs:
	@mkdir -p $(BUILD_DIR)/docs
	doxygen ci/doxy/docs.cfg

.PHONY: docs-view
docs-view:
	xdg-open $(BUILD_DIR)/docs/html/index.html

.PHONY: docs-publish
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

#
# Commands useful for development
#
.PHONY: tags
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

# ... all of them
