#include <pybind11/pybind11.h>

#include "transport/include/block_channel.h"
#include "transport/include/ipc.h"

namespace py = pybind11;

using ChannelNameT = std::string;
using DataT = std::string;

PYBIND11_MODULE(transport_py, m) {
  m.doc() = "The py-binding of the data transport library";

  py::class_<transport::Transport<DataT>>(m, "Transport");

  py::class_<transport::IPC<DataT>, transport::Transport<DataT>>(m, "IPC")
      .def(py::init<const ChannelNameT &>())
      .def("send", &transport::IPC<DataT>::Send)
      .def(
          "recv",
          [&](transport::IPC<DataT> &ipc, uint32_t time_out) {
            DataT data;
            ipc.Receive(&data, time_out);
            return py::bytes(data.data(), data.size());
          },
          py::arg("time_out") = 10000);

  py::class_<transport::BlockChannel<DataT>, transport::Transport<DataT>>(
      m, "BlockChannel")
      .def(py::init<const ChannelNameT &>())
      .def("send", &transport::BlockChannel<DataT>::Send)
      .def(
          "recv",
          [&](transport::BlockChannel<DataT> &channel, uint32_t time_out) {
            DataT data;
            channel.Receive(&data, time_out);
            return py::bytes(data.data(), data.size());
          },
          py::arg("time_out") = 10000);
}
