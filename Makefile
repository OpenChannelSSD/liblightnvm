BUILD_TYPE?=Release
BUILD_DIR?=build
INSTALL_PREFIX?=/usr/local
NVM_LIBRARY_SHARED?=ON
NVM_TESTS?=OFF
NVM_CLI?=ON

#
# Traditional build commands / make interface
#
default: make

.PHONY: debug
debug:
	$(eval BUILD_TYPE := Debug)

.PHONY: cmake_check
cmake_check:
	@cmake --version || (echo "\n** Please install 'cmake' **\n" && exit 1)

.PHONY: configure
configure: cmake_check
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake \
	-DNVM_LIBRARY_SHARED=$(NVM_LIBRARY_SHARED) \
	-DTESTS=$(NVM_TESTS) \
	-DCLI=$(NVM_CLI) \
	-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
	-DCMAKE_INSTALL_PREFIX:PATH=$(INSTALL_PREFIX) \
	$(CMAKE_AUX) \
	-G "Unix Makefiles" ../
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
	sudo apt-get --yes remove liblightnvm-* || true

.PHONY: clean
clean:
	rm -fr $(BUILD_DIR) || true
	rm -f tags || true

all: clean default install

.PHONY: dev_opts
dev_opts:
	$(eval NVM_TESTS := ON)
	$(eval NVM_CLI := ON)
	$(eval NVM_LIBRARY_SHARED := ON)

# Uinstall packages, clean build, builds lib, cli, tests, pkg and installs
.PHONY: dev
dev: uninstall-pkg clean dev_opts make-pkg install-pkg


.PHONY: tags
tags:
	ctags * -R .
	cscope -b `find . -name '*.c'` `find . -name '*.h'`

#
# Experimental section
#
.PHONY: doxy
doxy:
	@mkdir -p $(BUILD_DIR)/doc/doxy
	doxygen doc/doxy.cfg

.PHONY: doxy-view
doxy-view:
	xdg-open $(BUILD_DIR)/doc/doxy/html/index.html

.PHONY: sphinx
sphinx:
	@mkdir -p $(BUILD_DIR)/doc/sphinx/html
	@mkdir -p $(BUILD_DIR)/doc/sphinx/pdf
	sphinx-build -b html -c doc -E doc/src $(BUILD_DIR)/doc/sphinx/html

.PHONY: sphinx-view
sphinx-view:
	xdg-open $(BUILD_DIR)/doc/sphinx/html/index.html

.PHONY: doc-gen-capi
doc-gen-capi:
	python doc/gen/capi.py doc/src/capi/ --header include/liblightnvm.h

.PHONY: doc-gen-misc
doc-gen-qs:
	cd doc/src/quick_start && python ../../gen/cli.py ./

.PHONY: doc-gen-cli
doc-gen-cli:
	python doc/gen/cli.py doc/src/cli/

.PHONY: doc-gen-tut
doc-gen-tut:
	python doc/gen/cli.py doc/src/tutorial/

.PHONY: doc-gen-cmds
doc-gen-cmds: doc-gen-qs doc-gen-cli doc-gen-tut

.PHONY: doc
doc: doxy sphinx

.PHONY: doc-view-html
doc-view-html:
	xdg-open $(BUILD_DIR)/doc/sphinx/html/index.html

.PHONY: doc-view-html-tutorial
doc-view-html-tutorial:
	xdg-open $(BUILD_DIR)/doc/sphinx/html/tutorial/index.html

#.PHONY: doc-view-pdf
#doc-view-pdf:
#	xdg-open $(BUILD_DIR)/doc/sphinx/pdf/liblightnvm.pdf

.PHONY: doc-publish
doc-publish:
	rm -rf $(BUILD_DIR)/ghpages
	mkdir -p $(BUILD_DIR)/ghpages
	git clone -b gh-pages git@github.com:OpenChannelSSD/liblightnvm.git --single-branch $(BUILD_DIR)/ghpages
	cd $(BUILD_DIR)/ghpages && git rm -rf --ignore-unmatch .
	cp -r $(BUILD_DIR)/doc/sphinx/html/. $(BUILD_DIR)/ghpages/
	touch $(BUILD_DIR)/ghpages/.nojekyll
	cd $(BUILD_DIR)/ghpages && git config user.name "Mr. Robot"
	cd $(BUILD_DIR)/ghpages && git config user.email "foo@example.com"
	cd $(BUILD_DIR)/ghpages && git add .
	cd $(BUILD_DIR)/ghpages && git commit -m "Autogen docs for `git rev-parse --short HEAD`."
	cd $(BUILD_DIR)/ghpages && git push origin --delete gh-pages
	cd $(BUILD_DIR)/ghpages && git push origin gh-pages

