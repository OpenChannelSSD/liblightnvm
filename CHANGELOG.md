# Changelog

## v0.1.6 (Upcoming)

 * Support for use of SGL via the `nvm_cmd` interface
   - Call `nvm_cmd_*` using command-opt `NVM_CMD_SGL`
   - data-ptr points to SGL/iovec compatible structure
   - meta-ptr points to SGL/iovec compatible structure

## v0.1.5 (Upcoming)

 * Rename `nvm_ret` to `nvm_ctx`
 * Expand backend interfaces
   - Add functions for ASYNC control: `init`, `prep`, `poke` and `wait`
   - Mediate via public API under prefix as `nvm_async` or `nvm_cmd_async`
 * Support for asynchronous command via the `nvm_cmd` interface
   - Setup `nvm_ctx` using e.g. `nvm_async_prep(cb_fnc, cb_arg)` returning `nvm_ctx`
   - Call `nvm_cmd_*` using command-opts `NVM_CMD_ASYNC` and `nvm_ctx`

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
