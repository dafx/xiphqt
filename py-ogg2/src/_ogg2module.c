#include <ogg2/ogg.h>

#include "general.h"
#include "_ogg2module.h"

#include "pyoggstreamstate.h"
#include "pyoggsyncstate.h"
#include "pyoggpacket.h"
#include "pyoggpackbuff.h"
#include "pyoggpage.h"

static PyMethodDef Ogg_methods[] = {
  {"OggPackBuff", PyOggPackBuffer_New,
   METH_VARARGS, PyOggPackBuffer_Doc},
  {"OggPackBuffB", PyOggPackBuffer_NewB,
   METH_VARARGS, PyOggPackBuffer_Doc},
  {"OggStreamState", PyOggStreamState_New, 
   METH_VARARGS, PyOggStreamState_Doc},
  {"OggSyncState", PyOggSyncState_New,
   METH_VARARGS, PyOggSyncState_Doc},
  {NULL, NULL}
};

static char docstring[] = "";

void
init_ogg2(void)
{
  PyObject *module, *dict, *Py_module_info;

  PyOggPackBuffer_Type.ob_type = &PyType_Type; 
  PyOggPacket_Type.ob_type = &PyType_Type;
  PyOggPage_Type.ob_type = &PyType_Type;
  PyOggStreamState_Type.ob_type = &PyType_Type;
  PyOggSyncState_Type.ob_type = &PyType_Type;
  
  module = Py_InitModule("_ogg2", Ogg_methods);
  dict = PyModule_GetDict(module);

  PyOgg_Error = PyErr_NewException("ogg2.OggError", NULL, NULL);
  PyDict_SetItemString(dict, "OggError", PyOgg_Error);
  Py_INCREF(PyOgg_Error);

  PyOggPage_Error = PyErr_NewException("ogg2.OggPageError", PyOgg_Error, NULL);
  PyDict_SetItemString(dict, "OggPageError", PyOggPage_Error);
  Py_INCREF(PyOggPage_Error);

  PyOggPacket_Error = PyErr_NewException("ogg2.OggPacketError", PyOgg_Error, NULL);
  PyDict_SetItemString(dict, "OggPacketError", PyOggPacket_Error);
  Py_INCREF(PyOggPacket_Error);

  PyModule_AddStringConstant(module, "__doc__", docstring);
  PyModule_AddStringConstant(module, "__version__", "2.0-pre_20040721");

  PyModule_AddIntConstant(module, "Ogg_Cont", OGG_CONT);
  PyModule_AddIntConstant(module, "Ogg_Discont", OGG_DISCONT);

  if (PyErr_Occurred())
    PyErr_SetString(PyExc_ImportError, "_ogg2: init failed");
}
