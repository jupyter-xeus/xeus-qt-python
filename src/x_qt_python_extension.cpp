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
#include "pybind11/embed.h"
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
#include "xeus-python/xdebugger.hpp"
#include "xeus-python/xutils.hpp"

namespace py = pybind11;


class  xpyqt_interpreter : public xpyt::interpreter
{
    public:
        xpyqt_interpreter(bool redirect_output_enabled, bool redirect_display_enabled )
        : xpyt::interpreter(redirect_output_enabled, redirect_display_enabled)
    {
        this->m_release_gil_at_startup = false;
    }

    void configure_impl() override{
        xpyt::interpreter::configure_impl();
        py::exec(R"pycode(

def _install_event_loop_caller():
    
    from IPython import get_ipython

    from qtpy import QtWidgets, QtCore
    import ctypes
    from contextlib import contextmanager


    class _EventLoopCaller(QtCore.QThread):
        DELAY = 100
    
        def __init__(self, *args, **kwargs):
            super().__init__(*args, **kwargs)
            self._run = True
            self.n_queued = 0
            self.mutex = QtCore.QMutex()
    
        def run(self):
        
            self.setPriority(QtCore.QThread.LowestPriority)

            cfunctype = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p)
            function_ptr = cfunctype(self.run_event_loop)
    
            while self._run:
                self.mutex.lock()
                if True or self.n_queued <= 0:
                    result = ctypes.pythonapi.Py_AddPendingCall(function_ptr, None)
                    if result == 0:
                        self.n_queued += 1
                self.mutex.unlock()
                QtCore.QThread.msleep(self.DELAY)
    
        def run_event_loop(self, c_ptr):
            self.mutex.lock()
            self.n_queued -= 1
            QtWidgets.QApplication.instance().processEvents()
            self.mutex.unlock()
            return 0


        @contextmanager
        def with_event_loop():
            loop = EventLoopCaller()
            try:
                loop.start()
                yield
            finally:
                loop._run = False
                loop.wait()
                pass
                

    class _QtEventLoopWrapper(object):
        def __init__(self):
            self.thread = None
        def pre_execute(self, *args, **kwargs):
            self.thread = _EventLoopCaller()
            self.thread.start()

            
        def post_execute(self, *args, **kwargs):
            if self.thread is not None:
                self.thread._run = False
                self.thread.wait()
    
    ip = get_ipython()
    event_loop_caller = _QtEventLoopWrapper()
    ip.events.register("pre_execute",event_loop_caller.pre_execute)
    ip.events.register("post_execute", event_loop_caller.post_execute)
        
_install_event_loop_caller()
del _install_event_loop_caller
        )pycode",py::globals());
    }
};



auto kernel_factory(bool redirect_output_enabled, bool redirect_display_enabled) -> std::unique_ptr<xeus::xkernel>
{


   
    // Registering SIGSEGV handler
#ifdef __GNUC__
    std::clog << "registering handler for SIGSEGV" << std::endl;
    signal(SIGSEGV, xpyt::sigsegv_handler);

    // Registering SIGINT and SIGKILL handlers
    signal(SIGKILL, xpyt::sigkill_handler);
#endif
    signal(SIGINT, xpyt::sigkill_handler);


    using context_type = xeus::xcontext_impl<zmq::context_t>;
    using context_ptr = std::unique_ptr<context_type>;
    context_ptr context = context_ptr(new context_type());

    // Instantiating the xeus xinterpreter
    using interpreter_ptr = std::unique_ptr<xeus::xinterpreter>;
    interpreter_ptr interpreter(new xpyqt_interpreter(redirect_output_enabled, redirect_display_enabled));


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
        .def(py::init(&kernel_factory), py::arg("redirect_output_enabled"), py::arg("redirect_display_enabled"))
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
