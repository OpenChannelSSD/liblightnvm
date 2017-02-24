.. _sec-cli-env:

Environment Variables
=====================

The following environment variables control the behavior of the CLI commands.

NVM_CLI_NOVERIFY
  When set, disables address verification
NVM_CLI_PMODE
  Control the plane-hint of ``nvm_addr`` and ``nvm_vblk``, values are:

  Disabled(0), DUAL(1), and QUAD(2)
NVM_CLI_BUF_PR
  When set, read/write commands will dump data to stdout
NVM_CLI_META_PR
  When set, read/write commands will dump meta-data (out-of-bound area) to
  stdout
NVM_CLI_ERASE_NADDRS_MAX
  Controls number of addresses pr. erase
NVM_CLI_READ_NADDRS_MAX
  Controls number of addresses pr. read
NVM_CLI_WRITE_NADDRS_MAX
  Controls number of addresses pr. write
