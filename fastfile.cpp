#include <Python.h>
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

    std::deque<PyObject*> linecache;
    std::ifstream fileifstream;

    FastFile(const char* filepath) : filepath(filepath), linecount(0), currentline(0)
    {
        // fprintf( stderr, "FastFile Constructor with filepath=%s\n", filepath );
        resetlines();
        fileifstream.open( filepath );

        if( fileifstream.fail() ) {
            std::cerr << "ERROR: FastFile failed to open the file '" << filepath << "'!" << std::endl;
        }
    }

    ~FastFile() {
        // fprintf( stderr, "~FastFile Destructor linecount %d currentline %d\n", linecount, currentline );
        fileifstream.close();
    }

    void resetlines() {
        currentline = -1;
    }

    std::string getlines(unsigned int linestoget) {
        std::stringstream stream;
        unsigned int current = 1;

        for( PyObject* line : linecache ) {
            ++current;
            const char* cppline = PyUnicode_AsUTF8( line );
            stream << std::string{cppline};

            if( linestoget < current ) {
                break;
            }
        }
        return stream.str();
    }

    bool getline() {
        std::string newline;

        if( std::getline( fileifstream, newline ) ) {
            linecount += 1;

            // fprintf( stderr, "linecount %d currentline %d newline '%s'\n", linecount, currentline, newline.c_str() ); fflush(stderr);
            PyObject* pythonobject = PyUnicode_DecodeUTF8( newline.c_str(), newline.size(), "replace" );

            // fprintf(stderr, "pythonobject '%d'\n", pythonobject); fflush(stderr);
            linecache.push_back( pythonobject );
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

        // fprintf( stderr, "boolline: %d linecount %d currentline %d\n", boolline, linecount, currentline );
        return boolline;
    }

    PyObject* call()
    {
        currentline += 1;
        // fprintf( stderr, "linecache.size %d linecount %d currentline %d\n", linecache.size(), linecount, currentline );

        if( currentline < linecache.size() )
        {
            return linecache[currentline];
        }
        else
        {
            if( !getline() )
            {
                // fprintf( stderr, "Raising StopIteration\n" );
                return PyUnicode_DecodeUTF8( "", 0, "replace" );
            }
        }
        // std::ostringstream contents; for( auto value : linecache ) contents << value; fprintf( stderr, "contents %s**\n**linecache.size %d linecount %d currentline %d (%d)\n", contents.str().c_str(), linecache.size(), linecount, currentline, linecache[currentline] );
        return linecache[currentline];
    }
};
