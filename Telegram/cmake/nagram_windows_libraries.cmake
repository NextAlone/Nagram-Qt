# Nagram Desktop: Qt 6 on Windows is configured with -system-webp and
# -system-zlib, so the application links the libwebp and zlib that prepare.py
# builds. cmake/external/qt/package.cmake names them where they are not:
# - libwebp is built into out/release-static/<architecture>, while
#   WebP_LIBRARY points at the x86 directory for every architecture;
# - zlib 1.3.2 names its static library libzs.lib (libzsd.lib in Debug), while
#   ZLIB_LIBRARY_RELEASE and ZLIB_LIBRARY_DEBUG name zlibstatic(d).lib.
# Qt's FindWrapWebP keeps an existing WrapWebP::WrapWebP and CMake's FindZLIB
# an existing ZLIB::ZLIB, so define both here, before package.cmake finds Qt.

if (WIN32 AND NOT DESKTOP_APP_USE_PACKAGED)
    if (NOT TARGET WrapWebP::WrapWebP)
        if (build_winarm)
            set(nagram_webp_architecture ARM64)
        elseif (build_win64)
            set(nagram_webp_architecture x64)
        else()
            set(nagram_webp_architecture x86)
        endif()
        set(nagram_webp_lib ${libs_loc}/libwebp/out/release-static/${nagram_webp_architecture}/lib)

        add_library(WrapWebP::WrapWebP INTERFACE IMPORTED)
        target_link_libraries(WrapWebP::WrapWebP
        INTERFACE
            ${nagram_webp_lib}/webp.lib
            ${nagram_webp_lib}/webpdemux.lib
            ${nagram_webp_lib}/webpmux.lib
        )
        target_include_directories(WrapWebP::WrapWebP
        INTERFACE
            ${libs_loc}/libwebp/src
        )
    endif()

    if (NOT TARGET ZLIB::ZLIB)
        add_library(ZLIB::ZLIB STATIC IMPORTED)
        set_target_properties(ZLIB::ZLIB PROPERTIES
            IMPORTED_LOCATION "${libs_loc}/zlib/Release/libzs.lib"
            IMPORTED_LOCATION_DEBUG "${libs_loc}/zlib/Debug/libzsd.lib"
            INTERFACE_INCLUDE_DIRECTORIES "${libs_loc}/zlib"
            INTERFACE_COMPILE_DEFINITIONS ZLIB_WINAPI
        )
    endif()
endif()
