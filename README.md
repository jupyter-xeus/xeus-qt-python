# xeus-qt-python

[![Build Status](https://github.com/jupyter-xeus/xeus-qt-python/workflows/CI/badge.svg)](https://github.com/jupyter-xeus/xeus-qt-python/actions)

You will need to compile [xeus-qt](https://github.com/jupyter-xeus/xeus-qt) and [xeus-qt-python](https://github.com/jupyter-xeus/xeus-qt-python) (this repository) from source.
First clone these two repositories under `path/to/xeus-qt` and `path/to/xeus-qt-python`, respectively (or whatever place you'd like).
Then enter in a terminal:

```console
micromamba create -f env.yml
micromamba activate xeus-qt-python
cd path/to/xeus-qt
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$CONDA_PREFIX
make install
cd path/to/xeus-qt-python
mkdir build
cd build
cmake .. -DCMAKE_PREFIX_PATH=$CONDA_PREFIX -DCMAKE_INSTALL_PREFIX=$CONDA_PREFIX
make
make install
cd ..
python kernel_widget.py
```

This launches JupyterLab in a Qt application. If you look at running kernels in the left tab, you will see a kernel named `qt-python`.
Right-click on it and choose "New Console for Kernel". This console won't be used, it is just a workaround to create a session with this kernel, so that we can then open a notebook using this session (this has to be improved in JupyterLab).

Now go to the file browser and double-click on the `my_notebook.ipynb` notebook.
In the "Select Kernel" menu, under "Use Kernel from Other Session" at the bottom, select "Console 1" (or whatever number, this is the console we previously created).
Now execute all the cells. The last one should display a button with the name "black magic" at the bottom of the Qt application.
If you click on it, it should display "hello from here" inside the last cell.

That's it, you've just connected your custom Qt application to Jupyter!
You added a button to your Qt application from a Jupyter notebook, which when clicked on, prints a message to a notebook cell output.
