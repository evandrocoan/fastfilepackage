#include <Python.h>
#include <cstdio>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <deque>

// https://stackoverflow.com/questions/56260096/how-to-improve-python-c-extensions-file-line-reading
// https://stackoverflow.com/questions/17237545/preprocessor-check-if-multiple-defines-are-not-defined
#if defined(__unix__)
    #define USE_POSIX_GETLINE

    #if defined(USE_STD_GETLINE) && USE_STD_GETLINE == 1
        #undef USE_POSIX_GETLINE
    #endif
#endif

struct FastFile
{
    size_t linebuffersize;
    const char* filepath;

    long long int linecount;
    long long int currentline;

    char* readline;
    PyObject* emtpycacheobject;
    std::deque<PyObject*> linecache;

#ifdef USE_POSIX_GETLINE
    FILE* cfilestream;
#else
    std::ifstream fileifstream;
#endif

    FastFile(const char* filepath) : linebuffersize(131072), filepath(filepath), linecount(0), currentline(0)
    {
        // fprintf( stderr, "FastFile Constructor with filepath=%s\n", filepath );
        resetlines();

        readline = (char*) malloc( linebuffersize );
        emtpycacheobject = PyUnicode_DecodeUTF8( "", 0, "replace" );

        if( readline == NULL ) {
            std::cerr << "ERROR: FastFile failed to alocate internal line buffer!" << std::endl;
        }

    #ifdef USE_POSIX_GETLINE
        cfilestream = fopen( filepath, "r" );

        if( cfilestream == NULL ) {
            std::cerr << "ERROR: FastFile failed to open the file '" << filepath << "'!" << std::endl;
        }
    #else
        fileifstream.open( filepath );

        if( fileifstream.fail() ) {
            std::cerr << "ERROR: FastFile failed to open the file '" << filepath << "'!" << std::endl;
        }
    #endif
    }

    ~FastFile() {
        // fprintf( stderr, "~FastFile Destructor linecount %d currentline %d\n", linecount, currentline );
        this->close();
        Py_XDECREF( emtpycacheobject );

        if( readline ) {
            free( readline );
        }

        for( PyObject* pyobject : linecache ) {
            Py_XDECREF( pyobject );
        }
    }

    void close() {
        // fprintf( stderr, "FastFile closing the file linecount %d currentline %d\n", linecount, currentline );
    #ifdef USE_POSIX_GETLINE
        if( cfilestream != NULL ) {
            fclose( cfilestream );
        }
    #else
        if( fileifstream.is_open() ) {
            fileifstream.close();
        }
    #endif
    }

    void resetlines(int linetoreset=-1) {
        currentline = linetoreset;
    }

    std::string getlines(unsigned int linestoget) {
        std::stringstream stream;
        unsigned int current = 1;
        const char* cppline;

        for( PyObject* line : linecache ) {
            ++current;
            cppline = PyUnicode_AsUTF8( line );
            stream << std::string{cppline};

            if( linestoget < current ) {
                break;
            }
            else {
                stream << '\n';
            }
        }
        return stream.str();
    }

    // https://stackoverflow.com/questions/11350878/how-can-i-determine-if-the-operating-system-is-posix-in-c
    bool _getline() {
    #ifdef USE_POSIX_GETLINE
        ssize_t nread;
        if( ( nread = getline( &readline, &linebuffersize, cfilestream ) ) != -1 )
        {
            linecount += 1;
            // fprintf( stderr, "_getline linecount %d currentline %d readline '%s'\n", linecount, currentline, readline.c_str() ); fflush( stderr );

            PyObject* pythonobject = PyUnicode_DecodeUTF8( readline, nread, "replace" );
            linecache.push_back( pythonobject );
            // fprintf( stderr, "_getline pythonobject '%d'\n", pythonobject ); fflush( stderr );

            // Py_XINCREF( emtpycacheobject );
            // linecache.push_back( emtpycacheobject );
            return true;
        }
    #else
        if( !fileifstream.eof() )
        {
            linecount += 1;
            fileifstream.getline( readline, linebuffersize );
            // fprintf( stderr, "_getline linecount %d currentline %d readline '%s'\n", linecount, currentline, readline.c_str() ); fflush( stderr );

            PyObject* pythonobject = PyUnicode_DecodeUTF8( readline, fileifstream.gcount(), "replace" );
            linecache.push_back( pythonobject );
            // fprintf( stderr, "_getline pythonobject '%d'\n", pythonobject ); fflush( stderr );

            // Py_XINCREF( emtpycacheobject );
            // linecache.push_back( emtpycacheobject );
            return true;
        }
    #endif
        return false;
    }

    bool next() {
        resetlines();

        if( linecache.size() ) {
            Py_XDECREF( linecache[0] );
            linecache.pop_front();
            return true;
        }
        bool boolline = _getline();

        // fprintf( stderr, "next boolline: %d linecount %d currentline %d\n", boolline, linecount, currentline );
        return boolline;
    }

    PyObject* call()
    {
        currentline += 1;
        // fprintf( stderr, "call linecache.size %d linecount %d currentline %d\n", linecache.size(), linecount, currentline );

        if( currentline < linecache.size() ) {
            return linecache[currentline];
        }
        else
        {
            if( !_getline() )
            {
                // fprintf( stderr, "Raising StopIteration\n" );
                return emtpycacheobject;
            }
        }
        // std::ostringstream contents; for( auto value : linecache ) contents << PyUnicode_AsUTF8( value ); fprintf( stderr, "call contents %s**\n**linecache.size %d linecount %d currentline %d (%d)\n", contents.str().c_str(), linecache.size(), linecount, currentline, linecache[currentline] );
        return linecache[currentline];
    }
};
