/***************************************************************************
* Copyright (c) 2018, Martin Renou, Johan Mabille, Sylvain Corlay, and     *
* Wolf Vollprecht                                                          *
* Copyright (c) 2018, QuantStack                                           *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/


#include "pybind11/pybind11.h"
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <signal.h>

#ifdef __GNUC__
#include <stdio.h>
#include <execinfo.h>
#include <stdlib.h>
#include <unistd.h>
#endif

#include "xq/xq_server.hpp"
#include "xq/xq_qt_poller.hpp"
#include "xeus/xkernel.hpp"
#include "xeus/xkernel_configuration.hpp"
#include "xeus/xhelper.hpp"

#include "xeus-python/xinterpreter.hpp"
#include "xeus-python/xinterpreter_raw.hpp"
#include "xeus-python/xdebugger.hpp"
#include "xeus-python/xutils.hpp"

namespace py = pybind11;


class  xpyqt_interpreter : public xpyt::interpreter
{
    public:
        xpyqt_interpreter()
        : xpyt::interpreter()
    {
        this->m_release_gil_at_startup = false;
    }
};

class  xpyqt_raw_interpreter : public xpyt::raw_interpreter
{
    public:
        xpyqt_raw_interpreter()
        : xpyt::raw_interpreter()
    {
        this->m_release_gil_at_startup = false;
    }
};


auto kernel_factory(const py::list args_list) -> std::unique_ptr<xeus::xkernel>
{
    // Extract cli args from Python object
    int argc = args_list.size();
    std::vector<char*> argv(argc);

    for (int i = 0; i < argc; ++i)
    {
        argv[i] = (char*)PyUnicode_AsUTF8(args_list[i].ptr());
    }

   
    // Registering SIGSEGV handler
#ifdef __GNUC__
    std::clog << "registering handler for SIGSEGV" << std::endl;
    signal(SIGSEGV, xpyt::sigsegv_handler);

    // Registering SIGINT and SIGKILL handlers
    signal(SIGKILL, xpyt::sigkill_handler);
#endif
    signal(SIGINT, xpyt::sigkill_handler);

    bool raw_mode = xpyt::extract_option("-r", "--raw", argc, argv.data());
    std::string connection_filename = xpyt::extract_parameter("-f", argc, argv.data());

    using context_type = xeus::xcontext_impl<zmq::context_t>;
    using context_ptr = std::unique_ptr<context_type>;
    context_ptr context = context_ptr(new context_type());

    // Instantiating the xeus xinterpreter
    using interpreter_ptr = std::unique_ptr<xeus::xinterpreter>;
    interpreter_ptr interpreter;
    if (raw_mode)
    {
        interpreter = interpreter_ptr(new xpyqt_raw_interpreter());
    }
    else
    {
        interpreter = interpreter_ptr(new xpyqt_interpreter());
    }

    using history_manager_ptr = std::unique_ptr<xeus::xhistory_manager>;
    history_manager_ptr hist = xeus::make_in_memory_history_manager();




    auto kernel = std::make_unique<xeus::xkernel>(xeus::get_user_name(),
                         std::move(context),
                         std::move(interpreter),
                         make_xq_server,
                         std::move(hist),
                         nullptr,
                         xpyt::make_python_debugger);

    const auto& config = kernel->get_config();
    return std::move(kernel);

}

PYBIND11_MODULE(xqtpython, m)
{
    py::class_<xeus::xkernel>(m, "xkernel")
        .def(py::init(&kernel_factory))
        .def("start", [](xeus::xkernel & kernel)->py::dict{
            kernel.start();
            auto config_dict = py::dict();
            const auto & config = kernel.get_config(); 
            config_dict["transport"] = config.m_transport;
            config_dict["ip"] = config.m_ip;
            config_dict["control_port"] = config.m_control_port;
            config_dict["shell_port"] = config.m_shell_port;
            config_dict["stdin_port"] = config.m_stdin_port;
            config_dict["iopub_port"] = config.m_iopub_port;
            config_dict["hb_port"] = config.m_hb_port;
            config_dict["signature_scheme"] = config.m_signature_scheme;
            config_dict["key"] = config.m_key;
            return config_dict;
        })
    ;

    m.doc() = "Xeus-qt-python kernel launcher";
}
#include "xeus-python/xdebugger.hpp"
