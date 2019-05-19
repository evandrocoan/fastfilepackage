#include <cstdio>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <deque>

struct FastFile
{
    const char* filepath;
    long long int linecount;
    long long int currentline;

    std::deque<std::string> linecache;
    std::ifstream fileifstream;

    FastFile(const char* filepath) : filepath(filepath), linecount(0), currentline(0)
    {
        // fprintf( stderr, "FastFile Constructor with filepath=%s\n", filepath );
        resetlines();
        fileifstream.open( filepath );
    }

    ~FastFile() {
        // printf( "~FastFile Destructor\n" );
        fileifstream.close();
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
                // stream << '\n';
            }
        }

        return stream.str();
    }

    bool getline() {
        std::string newline;

        if( std::getline( fileifstream, newline ) ) {
            linecount += 1;

            // newline.pop_back();
            linecache.push_back( newline );
            return true;
        }
        return false;
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
