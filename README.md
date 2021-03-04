# Hash File Name

This program takes a user-entered file name from the command line, sends it to the child process to compute the hash, and returns the resulting hash value of the file name to the parent process to output.

Hash programs used: md5sum, sha1sum, sha224sum, sha256sum, sha384sum, sha512sum

### How to run the program:
1. compile by using command `clang++ hashFileName.cpp -o hashFileName`
2. run by using command `./hashFileName <file name>`
