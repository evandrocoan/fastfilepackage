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
    long long int currentline;

    char* readline;
    FILE* cfilestream;
    std::string* emptystring;
    std::deque<std::string*> linecache;
    std::ifstream fileifstream;

    FastFile(const char* filepath) : linebuffersize(131072), filepath(filepath), linecount(0), currentline(0)
    {
        emptystring = new std::string{""};
        readline = (char*) malloc( linebuffersize );

        if( readline == NULL ) {
            std::cerr << "ERROR: FastFile failed to alocate internal line buffer!" << std::endl;
        }

        cfilestream = fopen( filepath, "r" );

        if( cfilestream == NULL ) {
            std::cerr << "ERROR: FastFile failed to open the file '" << filepath << "'!" << std::endl;
        }

        fileifstream.open( filepath );

        if( fileifstream.fail() ) {
            std::cerr << "ERROR: FastFile failed to open the file '" << filepath << "'!" << std::endl;
        }
    }

    ~FastFile() {
        this->close();
        delete emptystring;

        if( readline ) {
            free( readline );
            readline = NULL;
        }

        for( std::string* line : linecache ) {
            delete line;
        }
    }

    void close() {
        if( cfilestream != NULL ) {
            fclose( cfilestream );
            cfilestream = NULL;
        }

        if( fileifstream.is_open() ) {
            fileifstream.close();
        }
    }

    void resetlines(int linetoreset=-1) {
        currentline = linetoreset;
    }

    bool _getline() {
        if( !fileifstream.eof() )
        {
            linecount += 1;
            // fileifstream.getline( readline, linebuffersize );
            // linecache.push_back( new std::string{readline} );

            std::string* line = new std::string{};
            std::getline( fileifstream, *line );
            linecache.push_back( line );
            return true;
        }
        return false;
    }

    bool next() {
        resetlines();

        if( linecache.size() ) {
            delete linecache[0];
            linecache.pop_front();
            return true;
        }
        bool boolline = _getline();
        return boolline;
    }

    std::string* call()
    {
        currentline += 1;

        if( currentline < linecache.size() ) {
            return linecache[currentline];
        }
        else
        {
            if( !_getline() ) {
                return emptystring;
            }
        }
        return linecache[currentline];
    }
};

// g++ -o main.exe getline_cpp_performance.cpp -g -ggdb --std=c++11 && time ./main.exe
int main(int argc, char const *argv[])
{
    FastFile myfile{"./myfile.log"};
    LOG( 1, "Starting...");

    while( myfile.next() ) {
        // myfile.call();
    }
    LOG( 1, "Ending...");
}
