# Nagram Desktop: Qt 6 on Windows is configured with -system-webp, so its WebP
# image format plugin links the libwebp that prepare.py builds into
# out/release-static/<architecture>. cmake_helpers points WebP_LIBRARY at the
# x86 directory for every architecture, and the x64 and ARM64 builds then fail
# to open webp.lib. Qt's FindWrapWebP keeps a WrapWebP::WrapWebP that already
# exists, so define it here, before cmake/external/qt/package.cmake finds Qt.

if (WIN32 AND NOT DESKTOP_APP_USE_PACKAGED AND NOT TARGET WrapWebP::WrapWebP)
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
