Place prebuilt SDL dependencies here so the Visual Studio project can build on any PC
without absolute user-specific paths.

Expected layout:

external/
  SDL2/
    include/
      SDL.h
      ...
    lib/
      x64/
        SDL2.lib
        SDL2.dll
      x86/
        SDL2.lib
        SDL2.dll
  SDL2_image/
    include/
      SDL_image.h
      ...
    lib/
      x64/
        SDL2_image.lib
        SDL2_image.dll
        libjpeg-9.dll
        libpng16-16.dll
        libtiff-5.dll
        libwebp-7.dll
        zlib1.dll
      x86/
        SDL2_image.lib
        SDL2_image.dll
        libjpeg-9.dll
        libpng16-16.dll
        libtiff-5.dll
        libwebp-7.dll
        zlib1.dll

The project also falls back to DLL files in the repository root during the
post-build copy step, but headers and .lib files are still required for compile
and link.
