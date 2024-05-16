# pawn [![GitHub Build status](https://github.com/jan-kelemen/pawn/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/jan-kelemen/pawn/actions/workflows/ci.yml/badge.svg?branch=master)

## Building
Necessary build tools are:
* CMake 3.27 or higher
* Conan 2.2 or higher
  * See [installation instructions](https://docs.conan.io/2/installation.html)
* One of supported compilers:
  * Clang-18
  * GCC-13
  * Visual Studio 2022 (MSVC v193)

```
conan install . --profile=conan/clang-18 --profile=conan/dependencies --build=missing --settings build_type=Release
cmake --preset release
cmake --build --preset=release
```

Note: Override packages from Conan Center with updated ones from [jan-kelemen/conan-recipes](https://github.com/jan-kelemen/conan-recipes), this is mostly due to hardening or sanitizer options being incompatible with packages on Conan Center.
```
git clone git@github.com:jan-kelemen/conan-recipes.git
conan create conan-recipes/recipes/sdl/all --version 2.30.1
conan create conan-recipes/recipes/freetype/meson --version 2.13.2
conan create conan-recipes/recipes/pulseaudio/meson --version 17.0 # Linux only
```

And then execute the build commands.
