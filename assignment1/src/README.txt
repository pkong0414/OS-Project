README.txt
Created by: Patrick Kong

Project goal:
    The project goal is to create a lite version of the disk usage function available in the unix commands.

Creating the executable:
    type: $make

Cleaning the directory of objects and executable:
    type: $make clean

Future Works:
    This will need to be cleaned up. I intend to make it more modular so it is easier to work with in the future.

Invoking the solution:
     * Your solution will be invoked using the following command:
     * mydu [-h]
     * mydu [-a] [-B M | -b | -m] [-c] [-d N] [-H] [-L]
     * mydu [-s] <dir1> <dir2> ...
     * -a Write count for all files, not just directories. (Prints out both files and directories)
     * -B M Scale sizes by M before printing; for example, -BM prints size in units of 1,048,576 bytes.
     * -b Print size in bytes.
     * -c Print a grand total.
     * -d N Print the total for a directory only if it is N or fewer levels below the command line argument.
     * -h Print a help message or usage, and exit (usage implies outputs name of the executable invoke and all the arguments and their meaning)
     * -H Human readable; print size in human readable format, for example, 1K, 234M, 2G.
     * -L Dereference all symbolic links. By default, you will not dereference symbolic links.
     * -m Same as -B 1048576.
     * -s Display only a total for each argument.