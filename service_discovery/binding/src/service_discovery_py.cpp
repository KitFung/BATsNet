#include <pybind11/pybind11.h>

#include "service_discovery/include/service_discovery.h"

namespace py = pybind11;

PYBIND11_MODULE(service_discovery_py, m) {
  m.doc() = "The py-binding of the service discovery library";

  m.def("get_service_path", &service_discovery::GetServicePath,
        py::return_value_policy::copy);
  // Assume there are no python service node, so not create binding for them
  // temporary
}
