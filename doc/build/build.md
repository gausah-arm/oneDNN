Build from Source {#dev_guide_build}
====================================

## Download the Source Code

Download [oneDNN source code](https://github.com/uxlfoundation/oneDNN/archive/main.zip)
or clone [the repository](https://github.com/uxlfoundation/oneDNN.git).

~~~sh
git clone https://github.com/uxlfoundation/oneDNN.git
~~~

## Build the Library

Ensure that all [software dependencies](https://github.com/uxlfoundation/oneDNN#requirements-for-building-from-source)
are in place and have at least the minimal supported version.

The oneDNN build system is based on CMake. Use

- `CMAKE_INSTALL_PREFIX` to control the library installation location,

- `CMAKE_BUILD_TYPE` to select between build type (`Release`, `Debug`,
  `RelWithDebInfo`).

- `CMAKE_PREFIX_PATH` to specify directories to be searched for the
  dependencies located at non-standard locations.

See @ref dev_guide_build_options for detailed description of build-time
configuration options.

### Linux/macOS

#### GCC, Clang, or Intel oneAPI DPC++/C++ Compiler

- Set up the environment for the compiler

- Configure CMake and generate makefiles
~~~sh
mkdir -p build
cd build

# Uncomment the following lines to build with Clang
# export CC=clang
# export CXX=clang++

# Uncomment the following lines to build with Intel oneAPI DPC++/C++ Compiler
# export CC=icx
# export CXX=icpx
cmake .. <extra build options>
~~~

- Build the library
~~~sh
make -j$(nproc)
~~~

#### Intel oneAPI DPC++/C++ Compiler with SYCL runtime

- Set up the environment for Intel oneAPI DPC++/C++ Compiler
  using the `setvars.sh` script. The command below assumes you installed to the
  default folder. If you customized the installation folder, setvars.sh (Linux/macOS)
  is in your custom folder:
~~~sh
source /opt/intel/oneapi/setvars.sh
~~~

- Configure CMake and generate makefiles
~~~sh
mkdir -p build
cd build

export CC=icx
export CXX=icpx

cmake .. \
          -DDNNL_CPU_RUNTIME=SYCL \
          -DDNNL_GPU_RUNTIME=SYCL \
          <extra build options>
~~~

@note Open-source version of oneAPI DPC++ Compiler does not have the icx driver,
use clang/clang++ instead. Open-source version of oneAPI DPC++ Compiler may not
contain OpenCL runtime. In this case, you can use `OPENCLROOT` CMake option or
environment variable of the same name to specify path to the OpenCL runtime if
it is installed in a custom location.

- Build the library
~~~sh
make -j$(nproc)
~~~

#### GCC targeting AArch64 on x64 host

- Set up the environment for the compiler

- Configure CMake and generate makefiles
~~~sh
export CC=aarch64-linux-gnu-gcc
export CXX=aarch64-linux-gnu-g++
cmake .. \
          -DCMAKE_SYSTEM_NAME=Linux \
          -DCMAKE_SYSTEM_PROCESSOR=AARCH64 \
          -DCMAKE_LIBRARY_PATH=/usr/aarch64-linux-gnu/lib \
          <extra build options>
~~~

- Build the library
~~~sh
make -j$(nproc)
~~~

#### GCC with Arm Compute Library (ACL) on AArch64 host

- Set up the environment for the compiler

- Configure CMake and generate makefiles
~~~sh
export ACL_ROOT_DIR=<path/to/Compute Library>
cmake .. \
          -DDNNL_AARCH64_USE_ACL=ON \
          <extra build options>
~~~

- Build the library
~~~sh
make -j$(nproc)
~~~

### Windows

#### Microsoft Visual C++ Compiler

- Generate a Microsoft Visual Studio solution
~~~bat
mkdir build
cd build
cmake -G "Visual Studio 16 2019" ..
~~~

- Build the library
~~~bat
cmake --build . --config=Release
~~~

@note Currently, the oneDNN build system has limited support for multi-config
 generators. Build configuration is based on the `CMAKE_BUILD_TYPE` option
 (`Release` by default), and CMake must be rerun from scratch every time
 the build type changes to apply the new build configuration. You can choose
 a specific build type with the `--config` option (the solution file supports
 both `Debug` and `Release` builds), but it must refer to the same build type
 (`Release`, `Debug`, etc.) as selected with the `CMAKE_BUILD_TYPE` option.

@note You can also open `oneDNN.sln` to build the project from the
Microsoft Visual Studio IDE.

#### Intel oneAPI DPC++/C++ Compiler with SYCL Runtime

- Set up the environment for Intel oneAPI DPC++/C++ Compiler
  using the `setvars.bat` script. The command below assumes you installed to the
  default folder. If you customized the installation folder, setvars.bat
  is in your custom folder:
~~~bat
"C:\Program Files (x86)\Intel\oneAPI\setvars.bat"
~~~
or open `Intel oneAPI Commmand Prompt` instead.

- Configure CMake and generate Ninja project
~~~bat
mkdir build
cd build

:: Set C and C++ compilers
set CC=icx
set CXX=icx
cmake .. -G Ninja -DDNNL_CPU_RUNTIME=SYCL ^
                  -DDNNL_GPU_RUNTIME=SYCL ^
                  <extra build options>
~~~

@warning Intel oneAPI DPC++/C++ Compiler on Windows requires CMake v3.23 or later.

@warning Intel oneAPI DPC++/C++ Compiler does not support CMake's Microsoft Visual
Studio generator.

@note Open-source version of oneAPI DPC++ Compiler does not have the icx driver,
use clang/clang++ instead. Open-source version of oneAPI DPC++ Compiler may not
contain OpenCL runtime. In this case, you can use `OPENCLROOT` CMake option or
environment variable of the same name to specify path to the OpenCL runtime if
it is installed in a custom location.

- Build the library
~~~bat
cmake --build .
~~~

## Validate the Build

If the library is built for the host system, you can run unit tests using:
~~~sh
ctest
~~~

## Build documentation

- Install the requirements
~~~sh
conda env create -f ../doc/environment.yml
conda activate onednn-doc
~~~

- Build the documentation
~~~sh
cmake --build . --target doc
~~~

## Install library

Install the library, headers, and documentation
~~~sh
cmake --build . --target install
~~~
The install directory is specified by the [CMAKE_INSTALL_PREFIX](https://cmake.org/cmake/help/latest/variable/CMAKE_INSTALL_PREFIX.html)
cmake variable. When installing in the default directory, the above command
needs to be run with administrative privileges using `sudo` on Linux/Mac or a
command prompt run as administrator on Windows.
