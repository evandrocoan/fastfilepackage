#include <cstdio>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <deque>

#include <stdio.h>
#include <stdlib.h>

struct FastFile
{
    char newline[102400];
    const char* filepath;
    long long int linecount;
    long long int currentline;

    std::deque<std::string> linecache;
    FILE* filepointer;

    FastFile(const char* filepath) : filepath(filepath), linecount(0), currentline(0)
    {
        // fprintf( stderr, "FastFile Constructor with filepath=%s\n", filepath );
        resetlines();
        filepointer = fopen(filepath, "r");

        if( filepointer == NULL ) {
            perror("Error opening file");
            exit(EXIT_FAILURE);
        }
    }

    ~FastFile() {
        // fprintf( stderr, "~FastFile Destructor\n" );
        fclose( filepointer );
    }

    void resetlines() {
        currentline = -1;
    }

    std::string getlines(unsigned int linestoget) {
        std::stringstream stream;
        unsigned int current = 1;

        for( std::string line : linecache ) {
            ++current;
            stream << line;

            if( linestoget < current ) {
                break;
            }
            else {
                stream << '\n';
            }
        }

        return stream.str();
    }

    bool getline() {
        unsigned int index = 0;
        char readchar;
        bool hasnewlines = true;
        while( true )
        {
            if( index > sizeof( newline ) - 2 ) {
                break;
            }
            readchar = fgetc( filepointer );

            // if( readchar == '\r' ) fprintf( stderr, "Reading char '\\r'\n" ); else if( readchar == '\n' ) fprintf( stderr, "Reading char '\\n'\n" ); else fprintf( stderr, "Reading char '%c'\n", readchar );

            if( readchar == '\n' )
            {
                if( index > 1 && newline[index-1] == '\r' ) {
                    // fprintf( stderr, "Cleaning \\r\n" );
                    --index;
                }
                break;
            }

            newline[index] = readchar;

            if( readchar == EOF ) {
                // fprintf( stderr, "END OF FILE index %d currentline %d hasnewlines %d '%d'\n", index, currentline, hasnewlines, readchar );

                hasnewlines = !!index;
                break;
            }
            else {
                ++index;
            }
        }
        newline[index] = '\0';
        // if( ferror( filepointer ) ) { fprintf( stderr, "Error: Reading file index %d hasnewlines %d '%s'\n", index, hasnewlines, filepath ); }
        // if( feof( filepointer ) ) { fprintf( stderr, "End of the file index %d hasnewlines %d '%s'\n", index, hasnewlines, filepath ); }

        if( hasnewlines ) {
            std::string stdline{newline};
            linecache.push_back( stdline );
            // fprintf( stderr, "NEWLINE '%s'++\n++linecache.size %d currentline %d linecount %d\n", stdline.c_str(), linecache.size(), currentline, linecount );
        }

        linecount += 1;
        return hasnewlines;
    }

    bool next() {
        resetlines();

        if( linecache.size() ) {
            linecache.pop_front();
            return true;
        }

        bool boolline = getline();
        // fprintf( stderr, "boolline: %d\n", boolline );
        return boolline;
    }

    std::string call()
    {
        currentline += 1;
        // fprintf( stderr, "linecache.size: %zd, currentline: %zd\n", linecache.size(), currentline );

        if( currentline < linecache.size() )
        {
            return linecache[currentline];
        }
        else
        {
            if( !getline() )
            {
                // fprintf( stderr, "Raising StopIteration\n" );
                return "";
            }
        }
        // std::ostringstream contents; for( auto value : linecache ) contents << value; fprintf( stderr, "contents %s**\n**linecache.size %d currentline %d linecount %d\n", contents.str().c_str(), linecache.size(), currentline, linecount );
        return linecache[currentline];
    }
};

// g++ -o main.exe fastfile.cpp -g -ggdb && ./main.exe
// int main(int argc, char const *argv[])
// {
//     FastFile fastfile( "./sample.txt" );
//     fprintf( stderr, "call a: '%s'\n", fastfile.call().c_str() );
//     fprintf( stderr, "call b: '%s'\n", fastfile.call().c_str() );
//     fastfile.resetlines();
//     fprintf( stderr, "call c: '%s'\n", fastfile.call().c_str() );
//     fprintf( stderr, "call d: '%s'\n", fastfile.call().c_str() );
//     fprintf( stderr, "call e: '%s'\n", fastfile.call().c_str() );
//     fprintf( stderr, "call f: '%s'\n", fastfile.call().c_str() );
//     fprintf( stderr, "call g: '%s'\n", fastfile.call().c_str() );
// }
