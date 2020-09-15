//diskusage.c
//AUTHOR: Patrick Kong
//DATE: 9/13/2020

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
#include <sys/stat.h>
#include <string.h>

//macro
#ifndef PATH_MAX
#define PATH_MAX 255
#endif

//global character count
static int totalSizeCount = 0;

//Global flags
//help flag
static int hflag = 0;
//print all flag
static int aflag = 0;
//prints bytes in units of M
//Ex: -B 1000 (prints units of bytes in 1000)
static int Bflag = 0;
static char *MValue = NULL;
//prints bytes in units of bytes
static int bflag = 0;
//prints bytes in units of megabytes or mb
static int mflag = 0;
//prints the grand total
static int cflag = 0;
//prints only N or less total of sub-directory for the directory in the arg
static int dflag = 0;
static int *NValue = 0;
//prints size in human readable format, for example, 1K, 234M, 2G.
static int Hflag = 0;
//Dereference all symbolic links. By default, you will not dereference symbolic links.
static int Lflag = 0;
//note: in the case of 2 arguments where: -a -s is involved.
// we can make a choice between just displaying: -s ONLY and ignore the -a
// or
// report an error because we cannot display all and summarized at the same time.
static int sflag = 0;

int depthfirstapply( char *path, int depthCount );

int main( int argc, char** argv ) {
    char *rootpath = argv[ argc - 1 ];
    //printf( "argc: %d\npath: %s\n", argc, rootpath );


    /* Invoking the solution
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
     * When you use perror, please print some meaningful error messages. The format for error messages should be:
     * mydu: Error: Detailed error message
     */

    //handles the return value of getopt
    int c;
    int depthCount = 0;
    int argCount = 1;

    //this will take in the arguments and process them into flags for the methods.
    while ( ( c = getopt(argc, argv, "haB:bmcd:HLs" ) ) != -1 ) {
        switch (c) {
            case 'h':
                hflag = 1;
                argCount++;
                break;
            case 'a':
                aflag = 1;
                argCount++;
                break;
            case 'B':
                Bflag = 1;
                argCount++;
                printf( "MValue: '%s'\n", optarg );
                MValue = optarg;
                break;
            case 'b':
                bflag = 1;
                argCount++;
                break;
            case 'm':
                mflag = 1;
                argCount++;
                break;
            case 'c':
                cflag = 1;
                argCount++;
                break;
            case 'd':
                dflag = 1;
                argCount++;
                NValue = atoi( optarg );
                printf( "NValue: '%d'\n", NValue );
                break;
            case 'H':
                Hflag = 1;
                argCount++;
                break;
            case 'L':
                Lflag = 1;
                argCount++;
                break;
            case 's':
                sflag = 1;
                argCount++;
                break;
            default:
                abort();
        }
    }

    if ( hflag == 1 ){
        printf( "     * Invoking the solution:\n"
                "     * Your solution will be invoked using the following command:\n"
                "     * mydu [-h]\n"
                "     * mydu [-a] [-B M | -b | -m] [-c] [-d N] [-H] [-L] \n"
                "     * mydu [-s] <dir1> <dir2> ...\n"
                "     * -a Write count for all files, not just directories. (Prints out both files and directories)\n"
                "     * -B M Scale sizes by M before printing; for example, -BM prints size in units of 1,048,576 bytes.\n"
                "     * -b Print size in bytes.\n"
                "     * -c Print a grand total.\n"
                "     * -d N Print the total for a directory only if it is N or fewer levels below the command line argument.\n"
                "     * -h Print a help message or usage, and exit (usage implies outputs name of the executable invoke and all the arguments and their meaning)\n"
                "     * -H Human readable; print size in human readable format, for example, 1K, 234M, 2G.\n"
                "     * -L Dereference all symbolic links. By default, you will not dereference symbolic links.\n"
                "     * -m Same as -B 1048576.\n"
                "     * -s Display only a total for each argument.\n\n");
        return 0;
    }

    //experimental section.
    if( sflag == 0 ) {
        int op = depthfirstapply(rootpath, depthCount);
        //printf( "\ntraversal done!\nEnding Program...\n" );
    }
    else if( sflag == 1 ){
        struct stat statbuf;
        int i;
        for( i = argCount; i < argc ; i++ ) {
            rootpath = argv[ i ];
            //printf( "rootpath: %s\n", rootpath);
            if( lstat( rootpath, &statbuf ) != -1 ){
                printf( "%d %s\n", statbuf.st_size, rootpath );
            } else {
                perror( "File Stat Error!\n" );
                return 0;
            }
        }
    }

    if( cflag == 1 ){
        printf( "grand total size of files: %d bytes.\n", totalSizeCount );
    }

    //debugging output
    printf ("hflag = %d, aflag = %d, Bflag = %d MValue = %s, bflag = %d, mflag = %d, cflag = %d, dflag = %d NValue = %d,"
            " Hflag = %d, Lflag = %d, sflag = %d\n",
            hflag, aflag, Bflag, MValue, bflag, mflag, cflag, dflag, NValue, Hflag, Lflag, sflag);

}

int depthfirstapply( char *path, int depthCount ) {
    /* 1. Write a function called depthfirstapply that has the following prototype.
     *      int depthfirstapply(char *path, int pathfun(char *path1));
     *
     *    The depthfirstapply function traverses the tree, starting at path. It applies the pathfun function to each file
     *    that it encounters in the traversal. The depthfirstapply function returns the sum of the positive return values
     *    of pathfun, or -1 if it failed to traverse any subdirectory of the directory. An example of possible pathfun is
     *    the sizepathfun function specified in the next part.
     */

    //checking out all the errors.
    if( sflag == 1 && aflag == 1 ){
        perror( "Printing everything and trying to summarize everything does not work together.\n");
        return 0;
    }
    else if( Bflag == 1 && bflag == 1 ){
        perror( "correct invokation: ./btTest [ -B MValue | -m | -b ] \n" );
        return 0;
    }
    else if( Bflag == 1 && MValue == NULL ){
        perror( "-B requires an integer of units. EX: -B100 | -B 100 \n");
        return 0;
    }

    //This will work but will need to add a little bit in order to do the traversals!
    //printf( "depthCount: %d\n", depthCount );
    depthCount = depthCount + 1;
    struct dirent *direntp;
    DIR *dirp;
    struct stat statbuf;
    char newPath[PATH_MAX];
    char symPath[PATH_MAX];
    int link;
    double result;
    char readUnit;
    //strcat( path, "/" );

    //early kicking out if depth is greater than NValue
    if( dflag == 1 && depthCount > NValue ){
        //printf( "depthCount: %d\n NValue: %d\n", depthCount, NValue );
        //printf( "jumping out of depth!\n" );
        return 0;
    }

    if ( (dirp = opendir( path ) ) == NULL ) {
        perror ( "Failed to open directory." );
        return 1;
    }

    while ( ( direntp = readdir( dirp ) ) != NULL ){
        if( ( strcmp( direntp->d_name, "." ) != 0 ) && ( strcmp( direntp->d_name, ".." ) != 0 ) ){
            sprintf( newPath, "%s/%s", path, direntp->d_name );
            if( lstat( newPath, &statbuf ) == -1 ) {
                perror( "File Stat Error!\n" );
                return 0;
            }
            else {
                if( cflag == 1 ){
                    totalSizeCount += (int)statbuf.st_size;
                }
                //trouble dereferencing the sym links.
                if( Lflag == 1 ){
                    //symlinks are either folders or files.
                    //This conditional assumes aflag is off
                    if ((S_ISLNK(statbuf.st_mode) == 1) ) {
                        stat( newPath, &statbuf );
                        if((S_ISDIR(statbuf.st_mode) == 1)) {
                            if (link = (readlink(newPath, symPath, sizeof(symPath) ) ) == -1) {
                                perror("readlink ERROR.\n");
                            } else {
                                if (Bflag == 1) {
                                    //prints in values of megabytes
                                    printf("%f / $s %s is a Symbolic Link -> %s directory.\n", (float) (statbuf.st_size / atoi(MValue)),
                                           MValue, newPath, symPath);
                                } else if (bflag == 1) {
                                    printf("%d b %s is a Symbolic Link -> %s directory.\n", statbuf.st_size, newPath, symPath);
                                } else if (mflag == 1) {
                                    printf("%f mb %s is a Symbolic Link -> %s directory.\n", (float) (statbuf.st_size / 1048576), newPath, symPath);
                                } else if( Hflag == 1 ){
                                    int i;
                                    for( i = 0; i < 3; i++) {
                                        if( i == 0 ) {
                                            result = (double) (statbuf.st_size / 1073741824);
                                            if( result > 1 ) {
                                                readUnit = 'G';
                                                break;
                                            }
                                        }
                                        if( i == 1 ) {
                                            result = (double) (statbuf.st_size / 1048576);
                                            if( result > 1 ) {
                                                readUnit = 'M';
                                                break;
                                            }
                                        }
                                        if( i == 2 ) {
                                            result = (double) (statbuf.st_size / 1000);
                                            readUnit = 'K';
                                            break;
                                        }

                                    }
                                    printf("%f %c %s is a Symbolic Link -> %s directory.\n", result, readUnit, newPath, symPath);
                                }
                            }
                        }
                    }
                    if( aflag == 1 ) {
                        //This conditional assumes aflag is on
                        lstat( newPath, &statbuf );
                        if ( (S_ISLNK(statbuf.st_mode) == 1) ) {
                            if (link = (readlink(newPath, symPath, sizeof(symPath) ) ) == -1) {
                                perror("readlink ERROR.\n");
                            } else {
                                if (Bflag == 1) {
                                    //prints in values of megabytes
                                    printf("%f / $s %s is a Symbolic Link -> %s file.\n", (float) (statbuf.st_size / atoi(MValue)), MValue,
                                           newPath, symPath);
                                } else if (bflag == 1) {
                                    printf("%d b %s is a Symbolic Link -> %s file.\n", statbuf.st_size, newPath, symPath);
                                } else if (mflag == 1) {
                                    printf("%f mb %s is a Symbolic Link -> %s file.\n", (float) (statbuf.st_size / 1048576), newPath, symPath);
                                } else if( Hflag == 1 ){
                                    int i;
                                    for( i = 0; i < 3; i++) {
                                        if( i == 0 ) {
                                            result = (double) (statbuf.st_size / 1073741824);
                                            if( result > 1 ) {
                                                readUnit = 'G';
                                                break;
                                            }
                                        }
                                        if( i == 1 ) {
                                            result = (double) (statbuf.st_size / 1048576);
                                            if( result > 1 ) {
                                                readUnit = 'M';
                                                break;
                                            }
                                        }
                                        if( i == 2 ) {
                                            result = (double) (statbuf.st_size / 1000);
                                            readUnit = 'K';
                                            break;
                                        }

                                    }
                                    printf("%f %c %s is a Symbolic Link -> %s file.\n", result, readUnit, newPath, symPath);
                                }
                            }
                        }
                    }
                }
                if( aflag == 1 ) {
                    if (S_ISLNK(statbuf.st_mode) == 1 && Lflag == 0) {
                        if (Bflag == 1) {
                            //prints in values of megabytes
                            printf("%f / $s %s is a Symbolic Link.\n", (float) (statbuf.st_size / atoi(MValue)),
                                   MValue,
                                   newPath);
                        } else if (bflag == 1) {
                            printf("%d b %s is a Symbolic Link.\n", statbuf.st_size, newPath);
                        } else if (mflag == 1) {
                            printf("%f mb %s is a Symbolic Link.\n", (float) (statbuf.st_size / 1048576), newPath);
                        } else if( Hflag == 1 ){
                            int i;
                            for( i = 0; i < 3; i++) {
                                if( i == 0 ) {
                                    result = (double) (statbuf.st_size / 1073741824);
                                    if( result > 1 ) {
                                        readUnit = 'G';
                                        break;
                                    }
                                }
                                if( i == 1 ) {
                                    result = (double) (statbuf.st_size / 1048576);
                                    if( result > 1 ) {
                                        readUnit = 'M';
                                        break;
                                    }
                                }
                                if( i == 2 ) {
                                    result = (double) (statbuf.st_size / 1000);
                                    readUnit = 'K';
                                    break;
                                }

                            }
                            printf("%f %c %s is a Symbolic Link.\n", result, readUnit, newPath);
                        }
                    }
                    if( S_ISREG( statbuf.st_mode ) == 1 ){
                        if (Bflag == 1) {
                            //prints in values of megabytes
                            printf("%f / %s %s is a file.\n", (float) (statbuf.st_size / atoi(MValue)), MValue,
                                   newPath);
                        } else if (bflag == 1) {
                            printf("%d b %s is a file.\n", statbuf.st_size, newPath);
                        } else if (mflag == 1) {
                            printf("%f mb %s is a file.\n", (float) (statbuf.st_size / 1048576), newPath);
                        } else if( Hflag == 1 ){
                            int i;
                            for( i = 0; i < 3; i++) {
                                if( i == 0 ) {
                                    result = (double) (statbuf.st_size / 1073741824);
                                    if( result > 1 ) {
                                        readUnit = 'G';
                                        break;
                                    }
                                }
                                if( i == 1 ) {
                                    result = (double) (statbuf.st_size / 1048576);
                                    if( result > 1 ) {
                                        readUnit = 'M';
                                        break;
                                    }
                                }
                                if( i == 2 ) {
                                    result = (double) (statbuf.st_size / 1000);
                                    readUnit = 'K';
                                    break;
                                }

                            }
                            printf("%f %c %s is a file.\n", result, readUnit, newPath);
                        }
                    }
                }
                if( S_ISDIR( statbuf.st_mode ) == 1 ){
                    if( Bflag == 1 ){
                        //prints in values of megabytes
                        printf( "%f / %s %s is a directory.\n", (float)(statbuf.st_size/atoi( MValue )), MValue, newPath );
                    }
                    else if( bflag == 1 ){
                        printf( "%d b %s is a directory.\n", statbuf.st_size, newPath );
                    }
                    else if( mflag == 1 ){
                        printf( "%f mb %s is a directory.\n", (float)(statbuf.st_size / 1048576), newPath );
                    }else if( Hflag == 1 ){
                        int i;
                        for( i = 0; i < 3; i++) {
                            if( i == 0 ) {
                                result = (double) (statbuf.st_size / 1073741824);
                                if( result > 1 ) {
                                    readUnit = 'G';
                                    break;
                                }
                            }
                            if( i == 1 ) {
                                result = (double) (statbuf.st_size / 1048576);
                                if( result > 1 ) {
                                    readUnit = 'M';
                                    break;
                                }
                            }
                            if( i == 2 ) {
                                result = (double) (statbuf.st_size / 1000);
                                readUnit = 'K';
                                break;
                            }
                        }
                        printf("%f %c %s is a directory.\n", result, readUnit, newPath);
                    }
                    depthfirstapply( newPath, depthCount );
                }
            }
        }
    }
    while ( ( closedir( dirp ) == -1 ) && ( errno == EINTR ) );


    return 0;
}