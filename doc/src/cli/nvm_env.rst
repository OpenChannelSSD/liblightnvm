.. _sec-cli-env:

Environment Variables
=====================

The following environment variables control the behavior of the CLI commands.

NVM_CLI_NOVERIFY
  When set, disables address verification
NVM_CLI_PMODE
  Control the plane-hint of ``nvm_addr`` and ``nvm_vblk``, values are:

  Disabled(0x0), DUAL(0x1), and QUAD(0x2)
NVM_CLI_BE_ID
  Defaults to NVM_BE_ANY, controls which transport backend to use

  NVM_BE_IOCTL(0x1), NVM_BE_IOCTL_SYSFS(0x2)
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
