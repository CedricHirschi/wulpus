# WULPUS Python source files

This directory contains the source files for the WULPUS Python API in [`wulpus`](wulpus) and an example Jupyter notebook in [`wulpus_gui.ipynb`](wulpus_gui.ipynb).

## How to install dependencies

1. Install the [`uv`](https://docs.astral.sh/uv/) Python package and project manager
2. In the `sw` folder, open a terminal and run the following command to install the Python requirements:
    ```bash
    uv add pyproject.toml
    ```
3. Finally, run the following command to start the Jupyter Notebook:
    ```bash
    uv run jupyter notebook
    ```

### What is `uv`?

`uv` is a Python package and project manager that simplifies the process of installing and managing dependencies. It uses a `pyproject.toml` file to define the project's dependencies and settings. You can learn more about `uv` by visiting the [official documentation](https://docs.astral.sh/uv/).

## License

The source files are released under Apache v2.0 (`Apache-2.0`) license unless noted otherwise, please refer to [`LICENSE`](LICENSE) file for details.