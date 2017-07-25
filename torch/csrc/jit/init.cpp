#include <Python.h>

#include "THP.h"
#include "torch/csrc/jit/ir.h"
#include "torch/csrc/jit/graph_fuser.h"
#include "torch/csrc/jit/python_tracer.h"
#include "torch/csrc/jit/python_ir.h"


PyObject * THPJIT_initExtension(PyObject *_unused)
{
  PyObject *jit_module = PyImport_ImportModule("torch.jit");
  THPUtils_assert(jit_module, "class loader couldn't access "
          "torch.jit module");
  PyObject *jit_dict = PyModule_GetDict(jit_module);

  THPGraphClass = PyMapping_GetItemString(jit_dict,(char*)"Graph");
  THPUtils_assert(THPGraphClass, "couldn't find "
          "Graph class in torch.jit module");

  Py_RETURN_TRUE;
}

namespace {

using namespace torch::jit;

using pass_type = std::unique_ptr<Graph> (std::unique_ptr<Graph>);

template<pass_type optimizer>
PyObject * wrap_optimizer(PyObject *_unused, PyObject *py_graph) {
  HANDLE_TH_ERRORS
  THPUtils_assert(THPGraph_Check(py_graph), "expected a Graph instance");
  THPGraph *graph = (THPGraph*)py_graph;
  graph->cdata = optimizer(std::unique_ptr<Graph>{graph->cdata}).release();
  Py_RETURN_NONE;
  END_HANDLE_TH_ERRORS
}

struct PyMethodDef _THPJIT_methods[] = {
  {"_jit_init",       (PyCFunction)THPJIT_initExtension,      METH_NOARGS,  NULL},
  {"_tracer_enter",   (PyCFunction)THPTracer_enter,           METH_VARARGS, NULL},
  {"_tracer_exit",    (PyCFunction)THPTracer_exit,            METH_VARARGS, NULL},
  {"_jit_optim_fuse", (PyCFunction)wrap_optimizer<FuseGraph>, METH_O,       NULL},
  {NULL}
};

} // anonymous namespace

PyMethodDef* THPJIT_methods() {
    return _THPJIT_methods;
}