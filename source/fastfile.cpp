//https://docs.python.org/3/c-api/intro.html#include-files
#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <cstdio>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <deque>

struct FastFile {
    const char* filepath;

    long long int linecount;
    long long int currentline;

    PyObject* emtpycacheobject;
    std::deque<PyObject*> linecache;

    PyObject* iomodule;
    PyObject* openfile;
    PyObject* fileiterator;

    // https://stackoverflow.com/questions/25167543/how-can-i-get-exception-information-after-a-call-to-pyrun-string-returns-nu
    FastFile(const char* filepath) : filepath(filepath), linecount(0), currentline(0)
    {
        // fprintf( stderr, "FastFile Constructor with filepath=%s\n", filepath );
        resetlines();
        iomodule = PyImport_ImportModule( "io" );
        emtpycacheobject = PyUnicode_DecodeUTF8( "", 0, "replace" );

        if( emtpycacheobject == NULL ) {
            std::cerr << "ERROR: FastFile failed to create the empty string object (and open the file '"
                    << filepath << "')!" << std::endl;
            PyErr_Print();
            return;
        }

        if( iomodule == NULL ) {
            std::cerr << "ERROR: FastFile failed to import the io module (and open the file '"
                    << filepath << "')!" << std::endl;
            PyErr_Print();
            return;
        }
        PyObject* openfunction = PyObject_GetAttrString( iomodule, "open" );

        if( openfunction == NULL ) {
            std::cerr << "ERROR: FastFile failed get the io module open function (and open the file '"
                    << filepath << "')!" << std::endl;
            PyErr_Print();
            return;
        }
        openfile = PyObject_CallFunction( openfunction, "s", filepath, "s", "r", "i", -1, "s", "UTF8", "s", "replace" );
        PyObject* iterfunction = PyObject_GetAttrString( openfile, "__iter__" );
        Py_DECREF( openfunction );

        if( iterfunction == NULL ) {
            std::cerr << "ERROR: FastFile failed get the io module iterator function (and open the file '"
                    << filepath << "')!" << std::endl;
            PyErr_Print();
            return;
        }
        PyObject* openfileresult = PyObject_CallObject( iterfunction, NULL );
        Py_DECREF( iterfunction );

        if( openfileresult == NULL ) {
            std::cerr << "ERROR: FastFile failed get the io module iterator object (and open the file '"
                    << filepath << "')!" << std::endl;
            PyErr_Print();
            return;
        }
        fileiterator = PyObject_GetAttrString( openfile, "__next__" );
        Py_DECREF( openfileresult );

        if( fileiterator == NULL ) {
            std::cerr << "ERROR: FastFile failed get the io module iterator object (and open the file '"
                    << filepath << "')!" << std::endl;
            PyErr_Print();
            return;
        }
    }

    ~FastFile() {
        // fprintf( stderr, "~FastFile Destructor linecount %d currentline %d\n", linecount, currentline );
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
        // fprintf( stderr, "FastFile closing the file linecount %d currentline %d\n", linecount, currentline );
        PyObject* closefunction = PyObject_GetAttrString( openfile, "close" );

        if( closefunction == NULL ) {
            std::cerr << "ERROR: FastFile failed get the close file function for '"
                    << filepath << "')!" << std::endl;
            PyErr_Print();
            return;
        }

        PyObject* closefileresult = PyObject_CallObject( closefunction, NULL );
        Py_DECREF( closefunction );

        if( closefileresult == NULL ) {
            std::cerr << "ERROR: FastFile failed close open file '"
                    << filepath << "')!" << std::endl;
            PyErr_Print();
            return;
        }

        Py_DECREF( closefileresult );
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
                stream.seekp( -1, std::ios_base::end );
                stream << " ";
                break;
            }
        }
        return stream.str();
    }

    // https://stackoverflow.com/questions/56260096/how-to-improve-python-c-extensions-file-line-reading
    bool _getline() {
        PyObject* readline = PyObject_CallObject( fileiterator, NULL );

        if( readline != NULL ) {
            linecount += 1;
            // fprintf( stderr, "_getline linecount %d currentline %d readline '%s'\n", linecount, currentline, PyUnicode_AsUTF8( readline ) ); fflush( stderr );

            linecache.push_back( readline );
            // fprintf( stderr, "_getline readline '%d'\n", readline ); fflush( stderr );
            return true;
        }

        // PyErr_Print();
        PyErr_Clear();
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
