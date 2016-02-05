#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <png.h>
#include <fftw3.h>


int main(int argc, char* argv[]) {
    char outfile[255];

    if (argc < 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    strcpy(outfile, argv[1]);
    strcat(outfile, ".png");

    printf("Writing output to %s...\n", outfile);

    return EXIT_SUCCESS;
}
