//https://docs.python.org/3/c-api/intro.html#include-files
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "debugger.h"

#include <cstdio>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <deque>

// https://stackoverflow.com/questions/56260096/how-to-improve-python-c-extensions-file-line-reading
// https://stackoverflow.com/questions/17237545/preprocessor-check-if-multiple-defines-are-not-defined
#if defined(FASTFILE_GETLINE)

    #if FASTFILE_GETLINE == 0
        #undef FASTFILE_GETLINE
    #endif

    #if defined(__unix__)
        #define USE_POSIX_GETLINE

        #if FASTFILE_GETLINE == 1
            #undef USE_POSIX_GETLINE
        #endif
    #endif
#endif

struct FastFile {
    const char* filepath;

    PyObject* emtpycacheobject;
    std::deque<PyObject*> linecache;

    bool hasclosed;
    bool hasfinished;
    long long int linecount;
    long long int currentline;

#if defined(FASTFILE_GETLINE)
    char* readline;
    size_t linebuffersize;

    #ifdef USE_POSIX_GETLINE
        FILE* cfilestream;
    #else
        std::ifstream fileifstream;
    #endif
#else
    PyObject* iomodule;
    PyObject* openfile;
    PyObject* fileiterator;
#endif

    // https://stackoverflow.com/questions/25167543/how-can-i-get-exception-information-after-a-call-to-pyrun-string-returns-nu
    FastFile(const char* filepath) : filepath(filepath), hasclosed(false), hasfinished(false), linecount(0), currentline(-1)
    {
        LOG( 1, "Constructor with filepath=%s", filepath );
        emtpycacheobject = PyUnicode_DecodeUTF8( "", 0, "replace" );

        if( emtpycacheobject == NULL ) {
            std::cerr << "ERROR: FastFile failed to create the empty string object (and open the file '"
                    << filepath << "')!" << std::endl;
            PyErr_PrintEx(100);
            return;
        }

#if defined(FASTFILE_GETLINE)
        linebuffersize = 131072;
        readline = (char*) malloc( linebuffersize );

        if( readline == NULL ) {
            std::cerr << "ERROR: FastFile failed to alocate internal line buffer for '"
                    << filepath << "'!" << std::endl;
            return;
        }

    #ifdef USE_POSIX_GETLINE
        cfilestream = fopen( filepath, "r" );
        if( cfilestream == NULL ) {
            std::cerr << "ERROR: FastFile failed to open the file '" << filepath << "'!" << std::endl;
            return;
        }
    #else
        fileifstream.open( filepath );

        if( fileifstream.fail() ) {
            std::cerr << "ERROR: FastFile failed to open the file '" << filepath << "'!" << std::endl;
            return;
        }
    #endif
#else
        // https://stackoverflow.com/questions/47054623/using-python3-c-api-to-add-to-builtins
        iomodule = PyImport_ImportModule( "builtins" );

        if( iomodule == NULL ) {
            std::cerr << "ERROR: FastFile failed to import the io module (and open the file '"
                    << filepath << "')!" << std::endl;
            PyErr_PrintEx(100);
            return;
        }
        PyObject* openfunction = PyObject_GetAttrString( iomodule, "open" );

        if( openfunction == NULL ) {
            std::cerr << "ERROR: FastFile failed get the io module open function (and open the file '"
                    << filepath << "')!" << std::endl;
            PyErr_PrintEx(100);
            return;
        }
        // https://stackoverflow.com/questions/56482802/c-python-api-extensions-is-ignoring-openerrors-ignore-and-keeps-throwing-the
        openfile = PyObject_CallFunction( openfunction, "ssiss", filepath, "r", -1, "UTF8", "ignore" );

        if( openfile == NULL ) {
            std::cerr << "ERROR: FastFile failed to open the file'"
                    << filepath << "'!" << std::endl;
            PyErr_PrintEx(100);
            return;
        }
        PyObject* iterfunction = PyObject_GetAttrString( openfile, "__iter__" );
        Py_DECREF( openfunction );

        if( iterfunction == NULL ) {
            std::cerr << "ERROR: FastFile failed get the io module iterator function (and open the file '"
                    << filepath << "')!" << std::endl;
            PyErr_PrintEx(100);
            return;
        }
        PyObject* openiteratorobject = PyObject_CallObject( iterfunction, NULL );
        Py_DECREF( iterfunction );

        if( openiteratorobject == NULL ) {
            std::cerr << "ERROR: FastFile failed get the io module iterator object (and open the file '"
                    << filepath << "')!" << std::endl;
            PyErr_PrintEx(100);
            return;
        }
        fileiterator = PyObject_GetAttrString( openfile, "__next__" );
        Py_DECREF( openiteratorobject );

        if( fileiterator == NULL ) {
            std::cerr << "ERROR: FastFile failed get the io module iterator object (and open the file '"
                    << filepath << "')!" << std::endl;
            PyErr_PrintEx(100);
            return;
        }
#endif
    }

    ~FastFile() {
        LOG( 1, "Destructor linecount %llu currentline %llu hasclosed %d", linecount, currentline, hasclosed );
        if( hasclosed ) {
            return;
        }
        this->close();
    }

    void close() {
        LOG( 1, "linecount %llu currentline %llu hasclosed %d", linecount, currentline, hasclosed );
        if( hasclosed ) {
            return;
        }

        hasclosed = true;
        Py_XDECREF( emtpycacheobject );

        for( PyObject* pyobject : linecache ) {
            Py_DECREF( pyobject );
        }

#if defined(FASTFILE_GETLINE)
        if( readline ) {
            free( readline );
            readline = NULL;
        }

        #ifdef USE_POSIX_GETLINE
            if( cfilestream != NULL ) {
                fclose( cfilestream );
                cfilestream = NULL;
            }
        #else
            if( fileifstream.is_open() ) {
                fileifstream.close();
            }
        #endif
#else
        PyObject* closefunction = PyObject_GetAttrString( openfile, "close" );

        if( closefunction == NULL ) {
            std::cerr << "ERROR: FastFile failed get the close file function for '"
                    << filepath << "')!" << std::endl;
            PyErr_PrintEx(100);
            return;
        }

        PyObject* closefileresult = PyObject_CallObject( closefunction, NULL );
        Py_DECREF( closefunction );

        if( closefileresult == NULL ) {
            std::cerr << "ERROR: FastFile failed close open file '"
                    << filepath << "')!" << std::endl;
            PyErr_PrintEx(100);
            return;
        }
        Py_DECREF( closefileresult );

        Py_XDECREF( iomodule );
        Py_XDECREF( openfile );
        Py_XDECREF( fileiterator );
#endif
    }

    void resetlines(int linetoreset=0) {
        currentline = linetoreset;
    }

    std::string getlines(unsigned int linestoget) {
        std::stringstream stream;

        if( linestoget ) {
        #if defined(FASTFILE_GETLINE)
            const char* cppline;
            unsigned int current = 1;

            for( PyObject* linepy : linecache ) {
                ++current;
                cppline = PyUnicode_AsUTF8( linepy );
                stream << std::string{cppline};

                if( linestoget < current ) {
                    break;
                }
                else {
                    stream << '\n';
                }
            }
        #else
            char* cpplinenonconst;
            const char* cppline;
            Py_ssize_t linesize;
            unsigned int current = 1;

            for( PyObject* linepy : linecache ) {
                ++current;
                cppline = PyUnicode_AsUTF8AndSize( linepy, &linesize );

                if( cppline[linesize-1] == '\n' ) {
                    cpplinenonconst = const_cast<char *>( cppline );
                    cpplinenonconst[linesize-1] = '\0';
                    stream << std::string{cpplinenonconst};
                }
                else {
                    stream << std::string{cppline};
                }

                if( linestoget < current ) {
                    break;
                }
            }
        #endif
        }
        return stream.str();
    }

    // https://stackoverflow.com/questions/56260096/how-to-improve-python-c-extensions-file-line-reading
    bool _getline() {
        // Fix StopIteration being raised multiple times because _getlines is called multiple times
        if( hasfinished ) { return false; }

#if defined(FASTFILE_GETLINE)
    #ifdef USE_POSIX_GETLINE
        ssize_t charsread;
        if( ( charsread = getline( &readline, &linebuffersize, cfilestream ) ) != -1 )
        {
            --charsread;
            linecount += 1;

            if( readline[charsread] == '\n' ) {
                readline[charsread] = '\0';
            }
            else {
                ++charsread;
            }

            PyObject* pythonobject = PyUnicode_DecodeUTF8( readline, charsread, "replace" );
            linecache.push_back( pythonobject );

            // Py_XINCREF( emtpycacheobject );
            // linecache.push_back( emtpycacheobject );
            LOG( 1, "linecount %llu currentline %llu readline '%p' '%s'", linecount, currentline, pythonobject, readline );
            return true;
        }
    #else
        if( !fileifstream.eof() )
        {
            linecount += 1;
            fileifstream.getline( readline, linebuffersize );

            PyObject* pythonobject = PyUnicode_DecodeUTF8( readline, fileifstream.gcount(), "replace" );
            linecache.push_back( pythonobject );

            // Py_XINCREF( emtpycacheobject );
            // linecache.push_back( emtpycacheobject );
            LOG( 1, "linecount %llu currentline %llu readline '%p' '%s'", linecount, currentline, pythonobject, readline );
            return true;
        }
    #endif
#else
        PyObject* readline = PyObject_CallObject( fileiterator, NULL );

        if( readline != NULL ) {
            linecount += 1;
            LOG( 1, "linecount %llu currentline %llu readline '%p' '%s'", linecount, currentline, readline, PyUnicode_AsUTF8( readline ) );

            linecache.push_back( readline );
            return true;
        }
#endif

        // PyErr_PrintEx(100);
        PyErr_Clear();
        hasfinished = true;
        return false;
    }

    bool next() {
        currentline = -1;

        if( linecache.size() ) {
            Py_DECREF( linecache[0] );
            linecache.pop_front();
            return true;
        }
        bool boolline = _getline();

        LOG( 1, "boolline: %d linecount %llu currentline %llu", boolline, linecount, currentline );
        return boolline;
    }

    PyObject* call()
    {
        currentline += 1;
        LOG( 1, "linecache.size %zd linecount %llu currentline %llu", linecache.size(), linecount, currentline );

        if( currentline < static_cast<long long int>( linecache.size() ) ) {
            return linecache[currentline];
        }
        else
        {
            if( !_getline() )
            {
                LOG( 1, "Raising StopIteration" );
                return emtpycacheobject;
            }
        }
        LOGCD( 1, std::ostringstream contents; for( auto value : linecache ) contents << PyUnicode_AsUTF8( value ); LOG( 1, "contents %s**\n**linecache.size %zd linecount %llu currentline %llu (%p)", contents.str().c_str(), linecache.size(), linecount, currentline, linecache[currentline] ) );
        return linecache[currentline];
    }
};
