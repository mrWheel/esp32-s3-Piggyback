# Working input solution

The interactive `basic` example uses a persistent byte-by-byte input loop over
the USB Serial/JTAG file descriptor.

Required behavior:

1. Write `Choice => ` before waiting for input.
2. Flush the USB Serial/JTAG TX FIFO with `fsync(STDOUT_FILENO)`.
3. Echo every received character immediately behind the prompt.
4. Flush each echoed character immediately.
5. Treat `CR` or `LF` as Enter, write one newline, and flush it.
6. Execute the command only after Enter, then print the next prompt.

Empty USB reads are treated as "no data yet" and do not terminate
`app_main()`. A `CRLF` sequence is treated as one Enter.