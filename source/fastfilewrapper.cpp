//https://docs.python.org/3/c-api/intro.html#include-files
#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <cstdio>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <deque>

#include "version.h"

typedef struct
{
    PyObject_HEAD
    PyObject* iomodule;
    PyObject* openfile;
    PyObject* fileiterator;
    PyObject* emtpycacheobject;

    long long int linecount;
    long long int currentline;

    std::deque<PyObject*>* linecache;
    const char* filepath;
}
PyFastFile;

// https://gist.github.com/physacco/2e1b52415f3a964ad2a542a99bebed8f
// https://stackoverflow.com/questions/48786693/how-to-wrap-a-c-object-using-pure-python-extension-api-python3
static PyModuleDef fastfilepackagemodule =
{
    // https://docs.python.org/3/c-api/module.html#c.PyModuleDef
    PyModuleDef_HEAD_INIT,
    "fastfilepackage", /* name of module */
    "Example module that wrapped a C++ object", /* module documentation, may be NULL */
    -1, /* size of per-interpreter state of the module, or -1 if the module keeps state in global variables. */
    NULL, /* PyMethodDef* m_methods */
    NULL, /* inquiry m_reload */
    NULL, /* traverseproc m_traverse */
    NULL, /* inquiry m_clear */
    NULL, /* freefunc m_free */
};

// initialize PyFastFile Object
static int PyFastFile_init(PyFastFile* self, PyObject* args, PyObject* kwargs) {
    char* filepath;
    if( !PyArg_ParseTuple( args, "s", &filepath ) ) {
        return -1;
    }

    // fprintf( stderr, "FastFile Constructor with filepath=%s\n", filepath );
    self->linecount = 0;
    self->currentline = 0;
    self->linecache = new std::deque<PyObject*>();
    self->filepath = filepath;

    self->iomodule = PyImport_ImportModule( "io" );
    self->emtpycacheobject = PyUnicode_DecodeUTF8( "", 0, "replace" );

    if( self->emtpycacheobject == NULL ) {
        std::cerr << "ERROR: FastFile failed to create the empty string object (and open the file '"
                << self->filepath << "')!" << std::endl;
        PyErr_Print();
        return -1;
    }

    if( self->iomodule == NULL ) {
        std::cerr << "ERROR: FastFile failed to import the io module (and open the file '"
                << self->filepath << "')!" << std::endl;
        PyErr_Print();
        return -1;
    }
    PyObject* openfunction = PyObject_GetAttrString( self->iomodule, "open" );

    if( openfunction == NULL ) {
        std::cerr << "ERROR: FastFile failed get the io module open function (and open the file '"
                << self->filepath << "')!" << std::endl;
        PyErr_Print();
        return -1;
    }
    self->openfile = PyObject_CallFunction( openfunction, "s", self->filepath, "s", "r", "i", -1, "s", "UTF8", "s", "replace" );
    PyObject* iterfunction = PyObject_GetAttrString( self->openfile, "__iter__" );
    Py_DECREF( openfunction );

    if( iterfunction == NULL ) {
        std::cerr << "ERROR: FastFile failed get the io module iterator function (and open the file '"
                << self->filepath << "')!" << std::endl;
        PyErr_Print();
        return -1;
    }
    PyObject* openfileresult = PyObject_CallObject( iterfunction, NULL );
    Py_DECREF( iterfunction );

    if( openfileresult == NULL ) {
        std::cerr << "ERROR: FastFile failed get the io module iterator object (and open the file '"
                << self->filepath << "')!" << std::endl;
        PyErr_Print();
        return -1;
    }
    self->fileiterator = PyObject_GetAttrString( self->openfile, "__next__" );
    Py_DECREF( openfileresult );

    if( self->fileiterator == NULL ) {
        std::cerr << "ERROR: FastFile failed get the io module iterator object (and open the file '"
                << self->filepath << "')!" << std::endl;
        PyErr_Print();
        return -1;
    }

    return 0;
}

// destruct the object
static PyObject* PyFastFile_close(PyFastFile* self, PyObject* args);

static void PyFastFile_dealloc(PyFastFile* self) {
    // fprintf( stderr, "~FastFile Destructor linecount %llu currentline %llu\n", self->linecount, self->currentline );

    // https://stackoverflow.com/questions/56212363/should-i-call-delete-or-py-xdecref-for-a-c-class-on-custom-dealloc-for-python
    PyFastFile_close( self, NULL );

    Py_XDECREF( self->emtpycacheobject );
    Py_XDECREF( self->iomodule );
    Py_XDECREF( self->openfile );
    Py_XDECREF( self->fileiterator );

    for( PyObject* pyobject : *self->linecache ) {
        Py_DECREF( pyobject );
    }

    delete self->linecache;
    Py_TYPE(self)->tp_free( (PyObject*) self );
}

// https://stackoverflow.com/questions/56260096/how-to-improve-python-c-extensions-file-line-reading
static bool PyFastFile__getline(PyFastFile* self) {
    PyObject* readline = PyObject_CallObject( self->fileiterator, NULL );

    if( readline != NULL ) {
        self->linecount += 1;
        // fprintf( stderr, "_getline linecount %llu currentline %llu readline '%s'\n", self->linecount, self->currentline, PyUnicode_AsUTF8( readline ) ); fflush( stderr );

        self->linecache->push_back( readline );
        // fprintf( stderr, "_getline readline '%p'\n", readline ); fflush( stderr );
        return true;
    }

    // PyErr_Print();
    PyErr_Clear();
    return false;
}

static PyObject* PyFastFile_tp_call(PyFastFile* self, PyObject* args, PyObject *kwargs)
{
    self->currentline += 1;
    // fprintf( stderr, "call linecache.size %lu linecount %llu currentline %llu\n", self->linecache->size(), self->linecount, self->currentline );

    if( self->currentline < static_cast<long long int>( self->linecache->size() ) ) {
        Py_INCREF( (*self->linecache)[self->currentline] );
        return (*self->linecache)[self->currentline];
    }
    else
    {
        if( !PyFastFile__getline( self ) )
        {
            // fprintf( stderr, "Raising StopIteration\n" );
            Py_INCREF( self->emtpycacheobject );
            return self->emtpycacheobject;
        }
    }
    // std::ostringstream contents; for( auto value : *self->linecache ) contents << PyUnicode_AsUTF8( value ); fprintf( stderr, "call contents %s**\n**linecache.size %lu linecount %llu self->currentline %llu (%p)\n", contents.str().c_str(), self->linecache->size(), self->linecount, self->currentline, (*self->linecache)[self->currentline] );
    Py_INCREF( (*self->linecache)[self->currentline] );
    return (*self->linecache)[self->currentline];
}

static PyObject* PyFastFile_tp_iter(PyFastFile* self, PyObject* args)
{
    Py_INCREF( self );
    return (PyObject*) self;
}

static PyObject* PyFastFile_iternext(PyFastFile* self, PyObject* args)
{
    self->currentline = -1;
    // fprintf( stderr, "next linecache.size %lu linecount %llu currentline %llu\n", self->linecache->size(), self->linecount, self->currentline );

    if( self->linecache->   size() ) {
        Py_DECREF( (*self->linecache)[0] );
        self->linecache->pop_front();

        PyObject* returnvalue = PyFastFile_tp_call( self, NULL, NULL );
        return returnvalue;
    }

    if( PyFastFile__getline( self ) ) {
        PyObject* returnvalue = PyFastFile_tp_call( self, NULL, NULL );
        return returnvalue;
    }
    return NULL;
}

static PyObject* PyFastFile_getlines(PyFastFile* self, PyObject* args)
{
    unsigned int linestoget;

    if( !PyArg_ParseTuple( args, "i", &linestoget ) ) {
        return NULL;
    }

    // https://stackoverflow.com/questions/36098984/python-3-3-c-api-and-utf-8-strings
    std::stringstream stream;
    unsigned int current = 1;
    const char* cppline;

    for( PyObject* line : *self->linecache ) {
        ++current;
        cppline = PyUnicode_AsUTF8( line );
        stream << std::string{cppline};

        if( linestoget < current ) {
            stream.seekp( -1, std::ios_base::end );
            stream << " ";
            break;
        }
    }

    std::string returnvalue = stream.str();
    return PyUnicode_DecodeUTF8( returnvalue.c_str(), returnvalue.size(), "replace" );
}

static PyObject* PyFastFile_resetlines(PyFastFile* self, PyObject* args)
{
    self->currentline = 0;
    Py_INCREF( Py_None );
    return Py_None;
}

static PyObject* PyFastFile_close(PyFastFile* self, PyObject* args)
{
    // fprintf( stderr, "FastFile closing the file linecount %llu currentline %llu\n", self->linecount, self->currentline );
    PyObject* closefunction = PyObject_GetAttrString( self->openfile, "close" );

    if( closefunction == NULL ) {
        std::cerr << "ERROR: FastFile failed get the close file function for '"
                << self->filepath << "')!" << std::endl;
        PyErr_Print();
        return NULL;
    }

    PyObject* closefileresult = PyObject_CallObject( closefunction, NULL );
    Py_DECREF( closefunction );

    if( closefileresult == NULL ) {
        std::cerr << "ERROR: FastFile failed close open file '"
                << self->filepath << "')!" << std::endl;
        PyErr_Print();
        return NULL;
    }
    Py_DECREF( closefileresult );

    Py_INCREF( Py_None );
    return Py_None;
}

static PyMethodDef PyFastFile_methods[] =
{
    { "close", (PyCFunction) PyFastFile_close, METH_VARARGS, "If the file object was open, close it" },
    { "getlines", (PyCFunction) PyFastFile_getlines, METH_VARARGS, "Return a string with `nth` cached lines" },
    { "resetlines", (PyCFunction) PyFastFile_resetlines, METH_NOARGS, "Reset the current line counter" },
    { "line", (PyCFunction) PyFastFile_tp_call, METH_NOARGS, "Return the next line or an empty string on the file end" },
    { "next", (PyCFunction) PyFastFile_iternext, METH_NOARGS, "Advances the iterator to the next line" },
    { NULL, NULL, 0, NULL }  /* Sentinel */
};

static PyTypeObject PyFastFileType =
{
    PyVarObject_HEAD_INIT( NULL, 0 )
    "fastfilepackage.FastFile" /* tp_name */
};

// create the module
PyMODINIT_FUNC PyInit_fastfilepackage(void)
{
    PyObject* thismodule;

    // https://docs.python.org/3/c-api/typeobj.html
    PyFastFileType.tp_call = (ternaryfunc) PyFastFile_tp_call;
    PyFastFileType.tp_iter = (getiterfunc) PyFastFile_tp_iter;
    PyFastFileType.tp_iternext = (iternextfunc) PyFastFile_iternext;

    PyFastFileType.tp_new = PyType_GenericNew;
    PyFastFileType.tp_basicsize = sizeof(PyFastFile);
    PyFastFileType.tp_dealloc = (destructor) PyFastFile_dealloc;
    PyFastFileType.tp_flags = Py_TPFLAGS_DEFAULT;
    PyFastFileType.tp_doc = "FastFile objects";

    PyFastFileType.tp_methods = PyFastFile_methods;
    // PyFastFileType.tp_members = PyFastFile_members;
    PyFastFileType.tp_init = (initproc) PyFastFile_init;

    if( PyType_Ready( &PyFastFileType) < 0 ) {
        return NULL;
    }

    thismodule = PyModule_Create(&fastfilepackagemodule);

    if( thismodule == NULL ) {
        return NULL;
    }

    // https://docs.python.org/3/c-api/arg.html#c.Py_BuildValue
    // https://stackoverflow.com/questions/3001239/define-a-global-in-a-python-module-from-a-c-api
    PyObject_SetAttrString( thismodule, "__version__", Py_BuildValue( "s", __version__ ) );

    // Add FastFile class to thismodule allowing the use to create objects
    Py_INCREF( &PyFastFileType );
    PyModule_AddObject( thismodule, "FastFile", (PyObject*) &PyFastFileType );
    return thismodule;
}
