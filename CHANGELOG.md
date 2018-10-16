# Changelog

The repository is tagged using semantic versioning, e.g. `v0.1.3`. The `master`
branch consist of the latest state of liblightnvm with possible hot-fixes since
the last version tag, consider `master` as a preview of the next version.

Changes are described in this file in a section named matching the version tag.
Sections with "(Upcoming)" describe changes on the roadmap for liblightnvm.

Changes on the `master` branch, from the latest version tag up to and including
HEAD can be subject to a git rebase.

## v0.1.8 (Upcoming)

* API / header cleanup
 - Consider adopting negative errno calling convention
 - Unify `nvm_bp`, `test_intf` and CLI ENV, args, setup and sub-commands
 - Replace `dev_path`/`dev_name` with `dev_ident`

## v0.1.7

Release is primarily fixes, cleanup, and expansion of test-suite

### C API

A bunch of fixes:

* Added 'constness' of read-only buffer variables
* Added missing @return descriptions
* Normalized @return | @returns -> @return
* Missing assignments of `errno`
* Missing error-indicator in `wrap->completed`
* Command-accessors

And a couple of additions:

* `nvm_addr_fill_crange`
* `nvm_dev_get_ns`
* `nvm_ret_clear`

### Testing

Four tests have been added which verify device behavior according to OCCSD 2.0:

* `test_compliance.c`
* `test_rules_read.c`
* `test_rules_reset.c`
* `test_rules_write.c`

## v0.1.6

Three new features and a couple of fixes/improvements.

### Experimental support for Scather/Gather List (SGL)

* Setup SGL (`nvm_sgl`) using `nvm_sgl_{alloc,add,free}`
* Call `nvm_cmd_{write,read}` using command option `NVM_CMD_SGL`
* Pass the `nvm_sgl` to `nvm_cmd_{write,read}` as data-pointer
* NOTES
  - meta is still a contiguous buffer
  - requires custom version of SPDK

### Zero-Copy support for SPDK backend

* Changed `nvm_buf_{alloc,free}`
  - Replaced `nvm_geo` with `nvm_dev` as argument, such that it can determine
not only alignment requirements but also DMA/non-DMA allocation requirements
  - Added `phys` argument such that the caller can get the physical address
upon call-time
* Added `nvm_dev` as argument to `nvm_buf_set`
* Added `nvm_buf_vtophys` retrieving the physical addr. of an IO buffer
* Added `nvm_buf_virt_{alloc,free}` explicitly allocating non-DMA buffers not
intended for IO

* lib: Refactored due to changes
* cli: Refactored due to changes
* lib: Refactored due to changes
* `nvm_be_spdk`:
 - Refactored due to changes
 - Removed allocations for DATA and META and removed transfers back and
 forth

### Towards reuse of command construction/handling

Added internal API for backends to perform command construction and completion
handling these are available via `nvm_cmd_wrap_{setup,cpl,term}`.

### Fixes / Improvements

* Added build of examples to build-system
* Fixed regression in return values for `NVM_BE_SPDK` with `NVM_CMD_SYNC`
* Added inclusion of libbsd queue.h it is used internally by `nvm_sgl`

## v0.1.5

### CLI

* Fixed meta-data issue on `nvm_cmd` CLI

Added `gen_as_geo` and `dev_as_geo` sub-commands to `nvm_addr` CLI. These are
useful for examining the hierarchical components of a given address.

Example:

```
# Create gen address using geometric components
nvm_addr s20_to_gen /dev/nvme0n1 0 2 10 42

# Show the geometric components of the gen-fmt-address
nvm_addr gen_as_geo /dev/nvme0n1 0x0002000a0000002a

# Convert gen-fmt-address to dev-fmt-address
nvm_addr gen2dev /dev/nvme0n1 0x0002000a0000002a

# Show the geometric components of the dev-fmt-address
# NOTICE: the dev-fmt-address is converted to gen-fmt before printing
nvm_addr dev_as_geo /dev/nvme0n1 0x000000000201402a
```

### API

* Added ASYNC CMD support
 - Usage examples provided in `REPOS/examples/`
* Added data-structure and helper functions for common "boiler-plate" code under
  `nvm_bp` prefix
* Added control on VBLK command options

### Backends

* Added initial support for ASYNC data commands
 - Added ASYNC support in SPDK backend for erase, write, read, and copy
 - Added ASYNC support in LBD backend for read and write commands
 - NOTE: Admin commands are NOT ASYNC
 - NOTE: IOCTL backend is NOT ASYNC
* Re-implemented SPDK backend for ASYNC support
* Added internal header for `NVM_BE_SPDK` facilitating derivative backends based
  on `NVM_BE_SPDK` in a style similar to how `NVM_BE_LBD` utilizes
  `NVM_BE_IOCTL` for most of the setup and command executions
* Fixed a regression introduced in v0.1.4 of the LBD backend

## v0.1.4

### struct nvm_ret

This structure is used for communication of low-level feedback and results.
Definition is changing, and more changes are coming as we implement support for
ASYNC commands.

* Changed representation of `nvm_ret` -- see `include/liblightnvm.h`
* Moved `nvm_ret` related functions into its own module -- see `src/nvm_ret.c`

A better name for this structure would be `nvm_ctx` and it will probably be
renamed so in v0.1.5.

### Functionality in the addressing API

The set of functions:

* `nvm_addr_erase`
* `nvm_addr_write`
* `nvm_addr_read`

Were the first functions performing IO in liblightnvm, they are superseded by
the equivalent `nvm_cmd_*` and from `v.0.1.4` removed from the library.

The functions:

* `nvm_addr_*2lba`
* `nvm_addr_lba2*`

Are removed as they were unused helper-functions for the backend formerly known
as LBA. The functions `nvm_addr_*2off` and `nvm_addr_off2*` are in use and kept
in the library with updated descriptions. See the backend changes for additional
info on the backend rename.

The remaining address-conversion function-descriptions were improved to clarify
usage.

### Backends

The backend interface is expanded for the construction and submission of DSM
de-allocate, SCALAR IO, and get/set feature. Also, a textual name is given to
each backend in the addition to the integral ID.
The IO submission functions are renamed to make room for explicitly controlling
the construction of the different IO commands. Inspect the details in
`include/nvm_be.h`.

The SYSFS backend is removed. It was used a means to perform a pseudo OCSSD 1.2
IDFY command.  That is, it constructed an OCSSD 1.2 idfy completion struct by
using values found in SYSFS.  This was used as a means to utilize a device
without admin-priviliges.  However, this did not work for OCSSD 2.0 the
nescesarry values are not available in SYSFS and the get-log-page also requires
admin-priviges. So the backend has a very narrow user-base and not worth
maintaining.

The LBA backend is renamed to LBD. The naming of the address conversion
functions, which aren't really used along with the name of this backend has
caused a lot of confusion. Updated function descriptions and renaming the
backend will help a lot in this area.

The SPDK backend now depends on SPDK v18.07, the SPDK backend had previously
relied only on `spdk_nvme_ctrlr_cmd_io_raw_with_md`, however, this function
currently does not allow construction of VECTOR erase when providing meta for
chunk-info. Thus, VECTOR erase is implemented using
`spdk_nvme_ocssd_ns_cmd_vector_reset`.

### Command Submission

The public `nvm_cmd_*` interface is expanded to expose the get/set feature
commands.

A set of `NVM_CMD_OPTS` is added to control how commands are constructed and
submitted. E.g. use `NVM_CMD_SCALAR` as flag to `nvm_cmd_write` and it will
construct and submit SCALAR write command. Use `NVM_CMD_VECTOR` for the
construction and submission of a VECTOR write command.

In upcoming releases the `NVM_CMD_OPTS` will also control ASYNC IO and support
for SGL / iovec for data and meta.

This behavior might be replaced with explicit function signatures for scalar vs
vector IO commands. However, in this release it is controlled by setting flags.

### Build-system

Fixed hardcoded path to SPDK. The docs in `doc/src/backends/nvm_be_spdk.rst`
describe how to install SPDK and which version is expected.

### DOC, CLI and Tests

Generally updated to reflect the changes above. That is, some of the CLI
commands will have new sub-commands due to expansions. Some will have others
removed due to deprecations.

Have a look at the online docs for a brief look at the CLI changes, they are
available with usage output.

### Changelog / roadmap

Added a CHANGELOG describing in prose, using slightly more words than commit
messages, what has changed and why. It also includes a brief roadmap/notice of
changes expected in upcoming releases.
