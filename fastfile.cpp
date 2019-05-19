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
        // printf( "FastFile Constructor with filepath=%s\n", filepath );
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

    bool getline() {
        std::string newline;

        if( std::getline( fileifstream, newline ) ) {
            linecount += 1;
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

        return getline();
    }

    std::string call()
    {
        currentline += 1;
        // printf( "linecache.size: %zd, currentline: %zd\n", linecache.size(), currentline );

        if( currentline < linecache.size() )
        {
            return linecache[currentline];
        }
        else
        {
            if( !getline() )
            {
                // printf( "Raising StopIteration\n" );
                return "";
            }
        }
        // std::ostringstream contents; for( auto value : linecache ) contents << value; printf( "contents %s**\n", contents.str().c_str() );
        return linecache[currentline];
    }
};

// int main(int argc, char const *argv[])
// {
//     FastFile fastfile( "./test.py" );
//     printf( "next: %d, call: %s", fastfile.next(), fastfile.call().c_str() );
//     printf( "call: %s", fastfile.call().c_str() );
//     printf( "call: %s", fastfile.call().c_str() );
//     fastfile.resetlines();
//     printf( "call: %s", fastfile.call().c_str() );
//     printf( "call: %s", fastfile.call().c_str() );
// }
