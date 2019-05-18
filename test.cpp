#include <Python.h>
#include <cstdio>

struct Voice
{
    double *mem;
    int fftSize;

    Voice(int fftSize)
    {
        printf("c++ constructor of voice\n");
        this->fftSize=fftSize;
        mem=new double[fftSize];
    }

    ~Voice()
    {
        delete [] mem;
    }

    int filter(int freq)
    {
        printf("c++ voice filter method\n");
        return (doubleIt(3));
    }

    int doubleIt(int i)
    {
        return 2*i;
    }
};

typedef struct
{
    PyObject_HEAD
    Voice * ptrObj;
} PyVoice;

static PyModuleDef voicemodule =
{
    PyModuleDef_HEAD_INIT,
    "voice",
    "Example module that wrapped a C++ object",
    -1,
    NULL, NULL, NULL, NULL, NULL
};

// initialize PyVoice Object
static int PyVoice_init(PyVoice *self, PyObject *args, PyObject *kwds)
{
    int fftSize;

    if (! PyArg_ParseTuple(args, "i", &fftSize))
        return -1;

    self->ptrObj=new Voice(fftSize);

    return 0;
}

// destruct the object
static void PyVoice_dealloc(PyVoice * self)
{
    delete self->ptrObj;
    Py_TYPE(self)->tp_free(self);
}

static PyObject * PyVoice_filter(PyVoice* self, PyObject* args)
{
    int freq;
    int retval;

    if (! PyArg_ParseTuple(args, "i", &freq))
        return Py_False;

    retval = (self->ptrObj)->filter(freq);
    return Py_BuildValue("i",retval);
}

static PyObject * PyVoice_doubleIt(PyVoice* self, PyObject* args)
{
    int freq;
    int retval;

    if (! PyArg_ParseTuple(args, "i", &freq))
        return Py_False;

    retval = (self->ptrObj)->doubleIt(freq);
    return Py_BuildValue("i",retval);
}

static PyMethodDef PyVoice_methods[] =
{
    { "filter", (PyCFunction)PyVoice_filter, METH_VARARGS, "filter the mem voice" },
    { "doubleIt", (PyCFunction)PyVoice_doubleIt, METH_VARARGS, "doubleIt the mem voice" },
    {NULL, NULL, 0, NULL}  /* Sentinel */
};

static PyTypeObject PyVoiceType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "voice.Voice" /* tp_name */
};

// create the module
PyMODINIT_FUNC PyInit_voice(void)
{
    PyObject* m;

    PyVoiceType.tp_new = PyType_GenericNew;
    PyVoiceType.tp_basicsize=sizeof(PyVoice);
    PyVoiceType.tp_dealloc=(destructor) PyVoice_dealloc;
    PyVoiceType.tp_flags=Py_TPFLAGS_DEFAULT;
    PyVoiceType.tp_doc="Voice objects";
    PyVoiceType.tp_methods=PyVoice_methods;

    //~ PyVoiceType.tp_members=Noddy_members;
    PyVoiceType.tp_init=(initproc)PyVoice_init;

    if (PyType_Ready(&PyVoiceType) < 0)
        return NULL;

    m = PyModule_Create(&voicemodule);
    if (m == NULL)
        return NULL;

    Py_INCREF(&PyVoiceType);

    // Add Voice object to the module
    PyModule_AddObject(m, "Voice", (PyObject *)&PyVoiceType);
    return m;
}
