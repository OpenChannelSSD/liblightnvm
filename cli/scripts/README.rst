 cli_perf.sh: Running the liblightnvm CLI perf test
====================================================

The following command will run the liblightnvm CLI perf test on device
"/dev/nvme0n1" using block 0 storing results in "cli_perf.out":

  ./cli_perf.sh /dev/nvme0n1 0 > cli_perf.out

If the command fails then:

 * Try a different block than 0, e.g. 42, or any other block.
 * Make sure you have permission to access the device.

The given block is spanned across channels 0 to 15 and three commands are run;
erase, write, and read. Results are reported as:

  {elapsed: 0.4478, mb: 2048.0000, mbsec: 4573.0915}

Where 'mb' is the amount of bytes erased, written, or read. 'elapsed' is the
amount of seconds it took to perform the operation, and 'mbsec' is the
bandwidth/throughput of the command in MB/sec.

A sample run is provided in the file "cli_perf.out"
