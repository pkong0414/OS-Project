//testDriver.c
//This will be used to develop and test functions going into the main src files.

/* Additional Notes:
 *
 * 3. Use depthfirstapply with pathfun given by sizepathfun to implement the following command.
 *      showtreesize pathname
 *
 *    I think we can just use this as default for mydu since we would already show the tree size in the program by
 *    default
 *
 *    Format:
 *    <tree size> <pathname>
 * 4. Write a command called mydu that is called with a command-line argument rootpath as follows.
 *      mydu rootpath
 *
 *    The mydu program calls a modified depthfirstapply with the function sizepathfun. It outputs the size of each
 *    directory followed by its pathname. The size of the directory does not count the size of subtrees of that
 *    directory. The program outputs the total size of the tree at the end and exists.
 */

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int depthfirstapply( char *path, int pathfun( char *path1 ) );
int pathfun( char *path );
int sizepathfun( char *path );

int main( int argc, char** argv ) {

    /* Invoking the solution
     * Your solution will be invoked using the following command:
     * mydu [-h]
     * mydu [-a] [-B M | -b | -m] [-c] [-d N] [-H] [-L] [-s] <dir1> <dir2> ...
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
     * When you use perror, please print some meaningful error messages. The format for error messages should be:
     * mydu: Error: Detailed error message
     */

    //
    int c;

    //help flag
    int hflag = 0;

    //print all flag
    int aflag = 0;

    //prints bytes in units of M
    //Ex: -B 1000 (prints units of bytes in 1000)
    int Bflag = 0;
    char *MValue = NULL;
    //prints bytes in units of bytes
    int bflag = 0;

    //prints bytes in units of megabytes or mb
    int mflag = 0;

    //prints the grand total
    int cflag = 0;

    //prints only N or less total of sub-directory for the directory in the arg
    int dflag = 0;
    char *NValue = NULL;

    //prints size in human readable format, for example, 1K, 234M, 2G.
    int Hflag = 0;

    //Dereference all symbolic links. By default, you will not dereference symbolic links.
    int Lflag = 0;

    //note: in the case of 2 arguments where: -a -s is involved.
    // we can make a choice between just displaying: -s ONLY and ignore the -a
    // or
    // report an error because we cannot display all and summarized at the same time.
    int sflag = 0;

    //this will take in the arguments and process them into flags for the methods.
    while ((c = getopt(argc, argv, "aB:bmcd:HLs")) != -1) {
        switch (c) {
            case 'h':
                hflag = 1;
                break;
            case 'a':
                aflag = 1;
                break;
            case 'B':
                Bflag = 1;
                printf( "M optarg: '%s'\n", optarg );
                MValue = optarg;
                break;
            case 'b':
                bflag = 1;
                break;
            case 'm':
                mflag = 1;
                break;
            case 'c':
                cflag = 1;
                break;
            case 'd':
                dflag = 1;
                printf( "N optarg: '%s'\n", optarg );
                NValue = optarg;
                break;
            case 'H':
                Hflag = 1;
                break;
            case 'L':
                Lflag = 1;
                break;
            case 's':
                sflag = 1;
                break;
            default:
                abort();
        }
    }

    //debugging output
    printf ("aflag = %d, Bflag = %d MValue = %s, bflag = %d, mflag = %d, cflag = %d, dflag = %d NValue = %s,"
            " Hflag = %d, Lflag = %d, sflag = %d\n",
            aflag, Bflag, MValue, bflag, mflag, cflag, dflag, NValue, Hflag, Lflag, sflag);

}

int depthfirstapply( char *path, int pathfun( char * path1 ) ) {
/* 1. Write a function called depthfirstapply that has the following prototype.
 *      int depthfirstapply(char *path, int pathfun(char *path1));
 *
 *    The depthfirstapply function traverses the tree, starting at path. It applies the pathfun function to each file
 *    that it encounters in the traversal. The depthfirstapply function returns the sum of the positive return values
 *    of pathfun, or -1 if it failed to traverse any subdirectory of the directory. An example of possible pathfun is
 *    the sizepathfun function specified in the next part.
 */

}

int sizepathfun( char *path ) {
/* 2. Write a function called sizepathfun that has the following prototype.
 *      int sizepathfun( char *path );
 *
 *    The sizepathfun function outputs path along other information obtained by stat for path. The sizepathfun returns
 *    the size in blocks of the file given by path or -1 if path does not correspond to an ordinary file.
 */

}