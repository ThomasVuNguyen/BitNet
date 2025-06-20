# Environment Setup Steps (from setup_env.py)

The `setup_env.py` script automates the process of setting up the environment for running BitNet models. It performs several key steps, configurable via command-line arguments.

## 1. Initialization and Argument Parsing

*   **Imports**: Necessary modules like `subprocess`, `os`, `platform`, `argparse`, `logging`, `shutil`, and `pathlib`.
*   **Logging**: Configures basic logging to `INFO` level and creates a log directory (default: `logs/`).
*   **Signal Handling**: Sets up a handler for `SIGINT` (Ctrl+C) to exit gracefully.
*   **Argument Parsing (`parse_args` function)**:
    *   Uses `argparse` to define and parse command-line arguments.
    *   Supported arguments:
        *   `--hf-repo` (`-hr`): Specifies the HuggingFace model repository to use (e.g., `"1bitLLM/bitnet_b1_58-large"`). Choices are defined in `SUPPORTED_HF_MODELS`.
        *   `--model-dir` (`-md`): Directory to save or load the model files (default: `"models"`).
        *   `--log-dir` (`-ld`): Directory to save log files for each step (default: `"logs"`).
        *   `--quant-type` (`-q`): Specifies the quantization type for the model (e.g., `"i2_s"`, `"tl1"`, `"tl2"`). Available choices depend on the system architecture (`SUPPORTED_QUANT_TYPES`) and defaults to `"i2_s"`.
        *   `--quant-embd`: A boolean flag. If set, token embeddings are quantized to `f16` during model conversion.
        *   `--use-pretuned` (`-p`): A boolean flag. If set, the script attempts to use pre-tuned kernel parameters for the selected model and architecture.

## 2. Main Execution Flow (`main` function)

The script executes the following functions in order:

1.  `setup_gguf()`: Installs GGUF support.
2.  `gen_code()`: Generates or copies necessary kernel code.
3.  `compile()`: Compiles the C++/CUDA source code.
4.  `prepare_model()`: Downloads and/or converts the model to the required GGUF format.

## 3. Key Setup Functions

### a. `setup_gguf()`

*   Installs the `gguf-py` Python package, which provides tools for working with GGUF (GPT-Generated Unified Format) files.
*   Command executed: `python -m pip install 3rdparty/llama.cpp/gguf-py`
*   Logs this step to `install_gguf.log` in the specified log directory.

### b. `gen_code()`

This function prepares the specialized LUT (Look-Up Table) kernels required for BitNet models.

*   Determines the system architecture (e.g., `arm64`, `x86_64`) using `system_info()`.
*   **If `args.use_pretuned` is True**:
    *   It attempts to copy pre-tuned kernel files from a `preset_kernels/<model_name>/` directory.
    *   For `arm64` and `quant_type="tl1"`: Copies `bitnet-lut-kernels-tl1.h` to `include/bitnet-lut-kernels.h` and `kernel_config_tl1.ini` to `include/kernel_config.ini`.
    *   For `arm64` and `quant_type="tl2"`: Copies `bitnet-lut-kernels-tl2.h` and `kernel_config_tl2.ini`.
    *   For `x86_64`: Copies `bitnet-lut-kernels-tl2.h`.
    *   If pre-tuned kernels for the specified model are not found, it logs an error and exits.
*   **If `args.use_pretuned` is False or for specific models**:
    *   It runs a Python code generation script (`utils/codegen_tl1.py` for `arm64` or `utils/codegen_tl2.py` for `x86_64`).
    *   The script is called with specific parameters (`--model`, `--BM`, `--BK`, `--bm`) tailored to the selected model (e.g., `bitnet_b1_58-large`, `Llama3-8B-1.58-100B-tokens`, `bitnet_b1_58-3B`).
    *   Logs this step to `codegen.log`.

### c. `compile()`

This function compiles the project using CMake.

*   Checks if `cmake` is installed and accessible.
*   Determines system architecture and OS.
*   **CMake Configuration**:
    *   Runs `cmake` to generate build files in the `build/` directory.
    *   Command: `cmake -B build [ARCH_EXTRA_ARGS] [OS_EXTRA_ARGS] -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++`
        *   `ARCH_EXTRA_ARGS`: Architecture-specific CMake flags from `COMPILER_EXTRA_ARGS` (e.g., `-DBITNET_ARM_TL1=ON`).
        *   `OS_EXTRA_ARGS`: OS-specific CMake flags from `OS_EXTRA_ARGS` (e.g., `["-T", "ClangCL"]` for Windows).
    *   Logs this step to `generate_build_files.log`.
*   **CMake Build**:
    *   Runs `cmake --build` to compile the code.
    *   Command: `cmake --build build --config Release -- -j 12` (parallel build with 12 jobs).
    *   Logs this step to `compile.log`.

### d. `prepare_model()`

This function handles downloading the model (if specified via HuggingFace repository) and converting it to the target GGUF format.

*   **Model Source**:
    *   If `args.hf_repo` is provided:
        *   Constructs the local model directory path: `args.model_dir / SUPPORTED_HF_MODELS[args.hf_repo]["model_name"]`.
        *   Creates the directory if it doesn't exist.
        *   Downloads the model files from the specified HuggingFace repository using `huggingface-cli download <hf_repo> --local-dir <model_dir_path>`.
        *   Logs this step to `download_model.log`.
    *   If `args.hf_repo` is not provided:
        *   It expects the model files to be already present in `args.model_dir`. If the directory doesn't exist, it logs an error and exits.
*   **GGUF Conversion**:
    *   Determines the target GGUF file path: `<model_dir_path>/ggml-model-<args.quant_type>.gguf`.
    *   If this GGUF file doesn't exist or is empty:
        *   Logs "Converting HF model to GGUF format...".
        *   **If `args.quant_type` starts with "tl" (e.g., "tl1", "tl2")**:
            *   Runs `utils/convert-hf-to-gguf-bitnet.py <model_dir_path> --outtype <args.quant_type>`.
            *   If `args.quant_embd` is True, adds `--quant-embd` to the command.
            *   Logs this step to `convert_to_tl.log`.
        *   **Else (for "i2_s" quantization)**:
            1.  Converts the model to `f32` GGUF format first: `python utils/convert-hf-to-gguf-bitnet.py <model_dir_path> --outtype f32`. Logs to `convert_to_f32_gguf.log`.
            2.  Quantizes the `f32` GGUF model to `I2_S` format using the `llama-quantize` executable (from the `build/bin/` or `build/bin/Release/` directory).
                *   Command (approximate): `./build/bin/llama-quantize [--token-embedding-type f16] <f32_model.gguf> <i2s_model.gguf> I2_S 1 [1]`
                *   The `--token-embedding-type f16` and the final `1` are added if `args.quant_embd` is True.
                *   Logs this step to `quantize_to_i2s.log`.
        *   Logs the path where the final GGUF model is saved.
    *   If the target GGUF file already exists and is not empty, it logs a message indicating so.

## 4. Helper Functions

*   `system_info()`: Returns a tuple of `(platform.system(), ARCH_ALIAS[platform.machine()])` (e.g., `('Linux', 'x86_64')`). `ARCH_ALIAS` maps various machine architecture strings to standardized ones.
*   `get_model_name()`: Retrieves the model name. If `args.hf_repo` is set, it uses the name from `SUPPORTED_HF_MODELS`. Otherwise, it derives the name from the base name of `args.model_dir`.
*   `run_command(command, shell=False, log_step=None)`: A utility function to execute system commands using `subprocess.run()`.
    *   It checks for successful execution (`check=True`).
    *   If `log_step` is provided, it redirects `stdout` and `stderr` of the command to a file named `<log_step>.log` in the `args.log_dir`.
    *   If an error occurs, it logs the error and exits the script.

This script provides a comprehensive solution for preparing the necessary components to run inferences with BitNet models, handling model acquisition, code generation for specific hardware, compilation, and model format conversion.