""" README:
- this is a proxy application for an actual qt application which wants to intergrate
  jupyter

- code to execute:

from kernel_widget import get_kernel_widget
from PyQt5.QtWidgets import  QPushButton
button = QPushButton()
button.setText("black magic")

def say_hello():
    print("hello from here")

button.clicked.connect(say_hello)

get_kernel_widget().layout.addWidget(button)
"""

import json
from pathlib import Path
import sys
import tempfile
import subprocess
import xqtpython
import socket
from contextlib import closing
from types import ModuleType

from PyQt5.QtCore import QUrl, QTimer
from PyQt5.QtWidgets import (
    QApplication,
    QVBoxLayout,
    QWidget,
)
from PyQt5.QtWebEngineWidgets import QWebEngineView


def find_free_port():
    with closing(socket.socket(socket.AF_INET, socket.SOCK_STREAM)) as s:
        s.bind(("", 0))
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        return s.getsockname()[1]


class KernelWidget(QWidget):
    """
    A webview widget showing a jupyterlab instance
    """

    def __init__(self, kernel_name, *args, **kwargs):
        super(KernelWidget, self).__init__(*args, **kwargs)

        self.layout = QVBoxLayout()
        self.setLayout(self.layout)

        self.kernel_name = kernel_name

        # browser
        self.browser = QWebEngineView()
        self.browser.setUrl(QUrl(""))

        # create tempdir for kernel.json
        self.kernel_file_dir = tempfile.TemporaryDirectory()

        self.layout.addWidget(self.browser)

        self.server_process = None
        self._start_jupyverse()
        self._make_widget_accessible()

        # start this once the app is running
        QTimer.singleShot(0, self._start_kernel)

    def _make_widget_accessible(self):
        # add a "virtual" module which makes
        # this widget available to users
        # in the nodebooks cell via
        #   from kernel_widget import get_kernel_widget
        #   get_kernel_widget()
        m = ModuleType("kernel_widget")
        sys.modules[m.__name__] = m
        m.__file__ = m.__name__ + ".py"

        def get_kernel_widget():
            return self

        m.get_kernel_widget = get_kernel_widget

    def _start_kernel(self):
        print("start kernel")
        self.kernel = xqtpython.xkernel(
            redirect_output_enabled=True, redirect_display_enabled=True
        )

        config = self.kernel.start()
        for k in ["shell_port", "control_port", "stdin_port", "iopub_port", "hb_port"]:
            config[k] = int(config[k])
        config["kernel_name"] = self.kernel_name

        kernel_file = Path(self.kernel_file_dir.name) / "kernel.json"

        with open(kernel_file, "w") as f:
            json.dump(config, f)

    def _start_jupyverse(self):
        # self.start_server_button.setDisabled(True)
        self.server_port = find_free_port()
        token = "my_token"

        args = [
            "jupyverse",
            "--set", "kernels.allow_external_kernels=true",
            "--set" ,f"kernels.external_connection_dir={str(self.kernel_file_dir.name)}",
            "--set" ,f"auth.token={token}",
            "--port", f"{self.server_port}",
        ]
        self.server_process = subprocess.Popen(
            args, shell=False
        )

        # we need to wait a tiny bit st the page is ready
        def setUrl():
            self.browser.setUrl(QUrl(f"http://127.0.0.1:{self.server_port}/?token={token}"))

        QTimer.singleShot(2000, setUrl)

    def closeEvent(self, event):
        # do stuff
        if self.server_process is not None:
            self.server_process.terminate()

        event.accept()


if __name__ == "__main__":
    app = QApplication(sys.argv)
    kernel_widget = KernelWidget(kernel_name="qt-python")

    kernel_widget.show()

    app.exec_()
