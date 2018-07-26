BUILD_TYPE?=Release
BUILD_DIR?=build
INSTALL_PREFIX?=/usr/local
CMAKE_OPTS?=

#
# liblightnvm uses cmake, however, to provide the traditional practice of:
#
# make
# make install
#
# The initial targets of this Makefile supports the behavior, instrumenting
# cmake with default options behind the scenes.
#
# Additional targets come after these which modify CMAKE build-options and other
# common practices associated with liblightnvm development.
#
default: build

.PHONY: cmake_check
cmake_check:
	@cmake --version || (echo "\n** Please install 'cmake' **\n" && exit 1)

.PHONY: configure
configure: cmake_check clean
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake			\
	-DCMAKE_BUILD_TYPE=$(BUILD_TYPE)		\
	-DCMAKE_INSTALL_PREFIX:PATH=$(INSTALL_PREFIX)	\
	$(CMAKE_OPTS)					\
	-G "Unix Makefiles" ../
	@echo "cmake is configured in '$(BUILD_DIR)'"
	@if [ "$(BUILD_DEB)" = "ON" ]; then		\
		touch $(BUILD_DIR)/build_deb;		\
	fi

.PHONY: build
build:
	cd $(BUILD_DIR) && make
	@ if [ -f "$(BUILD_DIR)/build_deb" ]; then	\
		cd $(BUILD_DIR) && make package;	\
	fi

.PHONY: install
install:
	cd $(BUILD_DIR) && make install

.PHONY: clean
clean:
	rm -fr $(BUILD_DIR) || true
	rm -f tags || true

.PHONY: clean-sys
clean-sys:
	@rm /usr/include/liblightnvm_cli.h || true
	@rm /usr/include/liblightnvm.h || true
	@rm /usr/lib/liblightnvm.a || true
	@rm /usr/lib/liblightnvm_cli.a || true
	@rm /usr/bin/nvm_* || true
	@rm /usr/local/include/liblightnvm_cli.h || true
	@rm /usr/local/include/liblightnvm.h || true
	@rm /usr/local/lib/liblightnvm_cli.a || true
	@rm /usr/local/bin/nvm_* || true

#
# Targets for assigning CMAKE build options, e.g.
#
# make debug cli_on tests_on spdk_on build
#
# This will enable debugging, the CLI, test programs, and the SPDK backend.
# Notice that the 'build' target must be specified as the last target
#
.PHONY: cli_on
cli_on:
	$(eval CMAKE_OPTS := ${CMAKE_OPTS} -DCLI=ON)

.PHONY: cli_off
cli_off:
	$(eval CMAKE_OPTS := ${CMAKE_OPTS} -DCLI=OFF)

.PHONY: tests_on
tests_on:
	$(eval CMAKE_OPTS := ${CMAKE_OPTS} -DTESTS=ON)

.PHONY: tests_off
tests_off:
	$(eval CMAKE_OPTS := ${CMAKE_OPTS} -DTESTS=OFF)

.PHONY: ioctl_on
ioctl_on:
	$(eval CMAKE_OPTS := ${CMAKE_OPTS} -DNVM_BE_IOCTL_ENABLED=ON)

.PHONY: ioctl_off
ioctl_off:
	$(eval CMAKE_OPTS := ${CMAKE_OPTS} -DNVM_BE_IOCTL_ENABLED=OFF)

.PHONY: sysfs_on
sysfs_on:
	$(eval CMAKE_OPTS := ${CMAKE_OPTS} -DNVM_BE_SYSFS_ENABLED=ON)

.PHONY: sysfs_off
sysfs_off:
	$(eval CMAKE_OPTS := ${CMAKE_OPTS} -DNVM_BE_SYSFS_ENABLED=OFF)

.PHONY: lba_on
lba_on:
	$(eval CMAKE_OPTS := ${CMAKE_OPTS} -DNVM_BE_LBA_ENABLED=ON)

.PHONY: lba_off
lba_off:
	$(eval CMAKE_OPTS := ${CMAKE_OPTS} -DNVM_BE_LBA_ENABLED=OFF)

.PHONY: spdk_on
spdk_on:
	$(eval CMAKE_OPTS := ${CMAKE_OPTS} -DNVM_BE_SPDK_ENABLED=ON)

.PHONY: spdk_off
spdk_off:
	$(eval CMAKE_OPTS := ${CMAKE_OPTS} -DNVM_BE_SPDK_ENABLED=OFF)

.PHONY: debug_on
debug_on:
	$(eval BUILD_TYPE := Debug)

.PHONY: debug_off
debug_off:
	$(eval BUILD_TYPE := Release)

.PHONY: deb_on
deb_on:
	$(eval BUILD_DEB := ON)

.PHONY: deb_off
deb_off:
	$(eval BUILD_DEB := OFF)

.PHONY: omp_on
omp_on:
	$(eval CMAKE_OPTS := ${CMAKE_OPTS} -DNVM_OMP_ENABLED=ON)

.PHONY: omp_off
omp_off:
	$(eval CMAKE_OPTS := ${CMAKE_OPTS} -DNVM_OMP_ENABLED=OFF)

#
# These targets works with tgz and deb packages, e.g.
#
# make install-deb
#
# Which will build a deb pkg and install it instead of copying it directly
# into the system paths. This is convenient as it is easier to purge it by
# running e.g.
#
# make uninstall-deb
#

.PHONY: install-deb
install-deb:
	dpkg -i $(BUILD_DIR)/*.deb

.PHONY: uninstall-deb
uninstall-deb:
	apt-get --yes remove liblightnvm-* || true

.PHONY: tags
tags:
	ctags * -R .
	cscope -b `find . -name '*.c'` `find . -name '*.h'`

#
# These are common practices during development
#

.PHONY: dev-spdk
dev-spdk: clean cli_on tests_on ioctl_on sysfs_on lba_on spdk_on debug_on deb_on configure build
	sudo make install-deb

.PHONY: dev
dev: clean cli_on tests_on ioctl_on sysfs_on lba_on spdk_off debug_on deb_on configure build
	sudo make install-deb

#
# DOC: This target generated API documentation based on source code
#
.PHONY: doc-gen-capi
doc-gen-capi:
	python doc/gen/capi.py doc/src/capi/ --header include/liblightnvm.h

#
# These targets generate documentation for quick-start guide, command-line
# interface, and tutorial. They use automated command execution for extracting
# command output, thus, they have to run on a system with an actual device
#

# Quick-Start Guide
.PHONY: doc-gen-qs
doc-gen-qs:
	cd doc/src/quick_start && python ../../gen/cli.py ./

# Tutorial
.PHONY: doc-gen-tut-s12
doc-gen-tut-s12:
	python doc/gen/cli.py doc/src/tutorial/tutorial-s12

.PHONY: doc-gen-tut-s20
doc-gen-tut-s20:
	python doc/gen/cli.py doc/src/tutorial/tutorial-s20

# Backends
.PHONY: doc-gen-backends
doc-gen-backends:
	python doc/gen/cli.py doc/src/backends

# CLI
.PHONY: doc-gen-cli-s12
doc-gen-cli-s12:
	python doc/gen/cli.py doc/src/cli/ --spec s12

.PHONY: doc-gen-cli-s20
doc-gen-cli-s20:
	python doc/gen/cli.py doc/src/cli/ --spec s20

# All of them
.PHONY: doc-gen-cmds
doc-gen-cmds: doc-gen-qs doc-gen-cli-s20 doc-gen-tut-s20

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

# Produce the sphinx stuff
.PHONY: doc
doc: doxy sphinx

.PHONY: doc-view-html
doc-view-html:
	xdg-open $(BUILD_DIR)/doc/sphinx/html/index.html

.PHONY: doc-view-html-tutorial
doc-view-html-tutorial:
	xdg-open $(BUILD_DIR)/doc/sphinx/html/tutorial/index.html

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
