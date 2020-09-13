//testDriver.c
//This will be used to develop and test functions going into the other c Files.

#include <stdio.h>
#include <string.h>



int main( int argc, char** argv ) {



    //ideal goal:
    //executable bt

    //single system call: [-h]
    //multiple system calls: [-L -d -g -i -p -s -t -u][-l]

    //don't forget to do a review on main parameters.

    // -L cases
    // This case deals with symbolic links.
    // Function will return the following:
    // 1 if the path is a symbolic link
    // 0 if the path does not have a link.
    // -1 if the path an error exists.

    //for this we'll be using:
    // int symlink( const char *path1, const char *path2 );
    // path1 contains the content of the link.
    // path2 contains the pathname of the link.

    //maybe don't use this as a function?

    char *path1, *path2;

    //idea will be to add system calls into the mix.
    //for now this will do for testing purposes.
    path1 = argv[ 1 ];

    printf( path1 );
    //path2 might require input. I'm willing to bet it is!
    path2 = './';


    int testLinks;

    testLinks = symlink( path1, path2 );

}