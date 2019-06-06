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

struct FastFile {
    const char* filepath;

    bool hasfinished;
    long long int linecount;
    long long int currentline;

    PyObject* emtpycacheobject;
    std::deque<PyObject*> linecache;

    PyObject* iomodule;
    PyObject* openfile;
    PyObject* fileiterator;

    // https://stackoverflow.com/questions/25167543/how-can-i-get-exception-information-after-a-call-to-pyrun-string-returns-nu
    FastFile(const char* filepath) : filepath(filepath), hasfinished(false), linecount(0), currentline(0)
    {
        LOG( 1, "Constructor with filepath=%s", filepath );
        resetlines();

        // https://stackoverflow.com/questions/47054623/using-python3-c-api-to-add-to-builtins
        iomodule = PyImport_ImportModule( "builtins" );
        emtpycacheobject = PyUnicode_DecodeUTF8( "", 0, "replace" );

        if( emtpycacheobject == NULL ) {
            std::cerr << "ERROR: FastFile failed to create the empty string object (and open the file '"
                    << filepath << "')!" << std::endl;
            PyErr_PrintEx(100);
            return;
        }

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
        openfile = PyObject_CallFunction( openfunction, "s", filepath, "s", "r", "i", -1, "s", "UTF8", "s", "ignore" );
        PyObject* iterfunction = PyObject_GetAttrString( openfile, "__iter__" );
        Py_DECREF( openfunction );

        if( iterfunction == NULL ) {
            std::cerr << "ERROR: FastFile failed get the io module iterator function (and open the file '"
                    << filepath << "')!" << std::endl;
            PyErr_PrintEx(100);
            return;
        }
        PyObject* openfileresult = PyObject_CallObject( iterfunction, NULL );
        Py_DECREF( iterfunction );

        if( openfileresult == NULL ) {
            std::cerr << "ERROR: FastFile failed get the io module iterator object (and open the file '"
                    << filepath << "')!" << std::endl;
            PyErr_PrintEx(100);
            return;
        }
        fileiterator = PyObject_GetAttrString( openfile, "__next__" );
        Py_DECREF( openfileresult );

        if( fileiterator == NULL ) {
            std::cerr << "ERROR: FastFile failed get the io module iterator object (and open the file '"
                    << filepath << "')!" << std::endl;
            PyErr_PrintEx(100);
            return;
        }
    }

    ~FastFile() {
        LOG( 1, "Destructor linecount %llu currentline %llu", linecount, currentline );
        this->close();
        Py_XDECREF( emtpycacheobject );
        Py_XDECREF( iomodule );
        Py_XDECREF( openfile );
        Py_XDECREF( fileiterator );

        for( PyObject* pyobject : linecache ) {
            Py_DECREF( pyobject );
        }
    }

    void close() {
        LOG( 1, "linecount %llu currentline %llu", linecount, currentline );
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
    }

    void resetlines(int linetoreset=-1) {
        currentline = linetoreset;
    }

    std::string getlines(unsigned int linestoget) {
        std::stringstream stream;

        if( linestoget ) {
            const char* cppline;
            unsigned int current = 1;

            for( PyObject* line : linecache ) {
                ++current;
                cppline = PyUnicode_AsUTF8( line );
                stream << std::string{cppline};

                if( linestoget < current ) {
                    stream.seekp( -1, std::ios_base::end );
                    stream << " ";
                    break;
                }
            }
        }
        return stream.str();
    }

    // https://stackoverflow.com/questions/56260096/how-to-improve-python-c-extensions-file-line-reading
    bool _getline() {
        // Fix StopIteration being raised multiple times because _getlines is called multiple times
        if( hasfinished ) { return false; }
        PyObject* readline = PyObject_CallObject( fileiterator, NULL );

        if( readline != NULL ) {
            linecount += 1;
            LOG( 1, "linecount %llu currentline %llu readline '%s'", linecount, currentline, PyUnicode_AsUTF8( readline ) );

            linecache.push_back( readline );
            LOG( 1, "readline '%p'", readline );
            return true;
        }

        // PyErr_PrintEx(100);
        PyErr_Clear();
        hasfinished = true;
        return false;
    }

    bool next() {
        resetlines();

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
