#include <Python.h>
#include "fastfile.cpp"

typedef struct
{
    PyObject_HEAD
    FastFile* cppobjectpointer;
}
PyFastFile;

// https://gist.github.com/physacco/2e1b52415f3a964ad2a542a99bebed8f
// https://stackoverflow.com/questions/48786693/how-to-wrap-a-c-object-using-pure-python-extension-api-python3
static PyModuleDef fastfilepackagemodule =
{
    PyModuleDef_HEAD_INIT,
    "fastfilepackage",
    "Example module that wrapped a C++ object",
    -1,
    NULL, NULL, NULL, NULL, NULL
};

// initialize PyFastFile Object
static int PyFastFile_init(PyFastFile *self, PyObject *args, PyObject *kwds) {
    char* filepath;

    if( !PyArg_ParseTuple(args, "s", &filepath) ) {
        return NULL;
    }

    self->cppobjectpointer = new FastFile( filepath );
    return 0;
}

// destruct the object
static void PyFastFile_dealloc(PyFastFile * self)
{
    delete self->cppobjectpointer;
    Py_TYPE(self)->tp_free(self);
}

static PyObject * PyFastFile_tp_call(PyFastFile* self, PyObject* args) {
    return (self->cppobjectpointer)->call();
}

static PyObject * PyFastFile_tp_iter(PyFastFile* self, PyObject* args)
{
    Py_INCREF(self);
    return (PyObject *) self;
}

static PyObject * PyFastFile_iternext(PyFastFile* self, PyObject* args)
{
    (self->cppobjectpointer)->resetlines();
    bool next = (self->cppobjectpointer)->next();

    // printf( "PyFastFile_iternext: %d\n", next );
    if( !( next ) )
    {
        PyErr_SetNone( PyExc_StopIteration );
        return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * PyFastFile_getlines(PyFastFile* self, PyObject* args)
{
    std::string retval;
    unsigned int linestoget;

    if( !PyArg_ParseTuple( args, "i", &linestoget ) ) {
        return NULL;
    }
    // https://stackoverflow.com/questions/36098984/python-3-3-c-api-and-utf-8-strings
    retval = (self->cppobjectpointer)->getlines( linestoget );
    return PyUnicode_DecodeUTF8( retval.c_str(), retval.size(), "replace" );
}

static PyObject * PyFastFile_resetlines(PyFastFile* self, PyObject* args)
{
    (self->cppobjectpointer)->resetlines();
    Py_INCREF(self);
    return (PyObject *) self;
}

static PyMethodDef PyFastFile_methods[] =
{
    { "getlines", (PyCFunction)PyFastFile_getlines, METH_VARARGS, "Return a string with `nth` cached lines" },
    { "resetlines", (PyCFunction)PyFastFile_resetlines, METH_NOARGS, "Reset the current line counter" },
    {NULL, NULL, 0, NULL}  /* Sentinel */
};

static PyTypeObject PyFastFileType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "FastFile" /* tp_name */
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
    PyFastFileType.tp_basicsize=sizeof(PyFastFile);
    PyFastFileType.tp_dealloc=(destructor) PyFastFile_dealloc;
    PyFastFileType.tp_flags=Py_TPFLAGS_DEFAULT;
    PyFastFileType.tp_doc="FastFile objects";
    PyFastFileType.tp_methods=PyFastFile_methods;

    //~ PyFastFileType.tp_members=Noddy_members;
    PyFastFileType.tp_init=(initproc)PyFastFile_init;

    if (PyType_Ready(&PyFastFileType) < 0) {
        return NULL;
    }

    thismodule = PyModule_Create(&fastfilepackagemodule);

    if (thismodule == NULL) {
        return NULL;
    }

    Py_INCREF(&PyFastFileType);

    // Add FastFile object to thismodule
    PyModule_AddObject(thismodule, "FastFile", (PyObject *)&PyFastFileType);
    return thismodule;
}
