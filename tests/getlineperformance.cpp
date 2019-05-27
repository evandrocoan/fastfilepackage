#include <cstdio>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <deque>

#define FASTFILE_DEBUGGER_INT_DEBUG_LEVEL 1
#include "../source/debugger.cpp"

struct FastFile
{
    size_t linebuffersize;
    const char* filepath;
    long long int linecount;

    char* readline;
    FILE* cfilestream;

    FastFile(const char* filepath) : linebuffersize(131072), filepath(filepath), linecount(0)
    {
        readline = (char*) malloc( linebuffersize );

        if( readline == NULL ) {
            std::cerr << "ERROR: FastFile failed to alocate internal line buffer!" << std::endl;
        }

        cfilestream = fopen( filepath, "r" );

        if( cfilestream == NULL ) {
            std::cerr << "ERROR: FastFile failed to open the file '" << filepath << "'!" << std::endl;
        }
    }

    ~FastFile() {
        this->close();

        if( readline ) {
            free( readline );
            readline = NULL;
        }
    }

    void close() {
        if( cfilestream != NULL ) {
            fclose( cfilestream );
            cfilestream = NULL;
        }
    }

    bool _getline() {
        ssize_t charsread;
        if( ( charsread = getline( &readline, &linebuffersize, cfilestream ) ) != -1 )
        {
            linecount += 1;
            readline[charsread-1] = '\0';
            return true;
        }
        return false;
    }
};

// g++ -o main.exe getlineperformance.cpp -g -ggdb --std=c++11 && time ./main.exe
int main(int argc, char const *argv[])
{
    FastFile myfile{"./myfile.log"};
    LOG( 1, "Starting...");

    while( myfile._getline() ) {
    }
    LOG( 1, "Ending...");
}
