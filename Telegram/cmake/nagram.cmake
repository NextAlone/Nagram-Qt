set(nagram_sources
    nagram/chats/layout.cpp
    nagram/chats/list_refresher.cpp
    nagram/chats/promotions.cpp
    nagram/compose/channel.cpp
    nagram/compose/confirm.cpp
    nagram/compose/format_toolbar.cpp
    nagram/compose/forward.cpp
    nagram/compose/placeholder.cpp
    nagram/compose/spacing.cpp
    nagram/compose/text.cpp
    nagram/core/language.cpp
    nagram/core/exchange.cpp
    nagram/core/options.cpp
    nagram/core/regex.cpp
    nagram/core/diagnostics.cpp
    nagram/core/relaunch.cpp
    nagram/core/updates.cpp
    nagram/core/version.cpp
    nagram/display/view_refresher.cpp
    nagram/export/range_model.cpp
    nagram/interface/main_menu.cpp
    nagram/interface/main_menu_model.cpp
    nagram/interface/notifications.cpp
    nagram/interface/text.cpp
    nagram/interface/appearance.cpp
    nagram/interface/app_icon.cpp
    nagram/chats/startup_folder.cpp
    nagram/chats/sort.cpp
    nagram/chats/recent_chats.cpp
    nagram/chats/local_pins.cpp
    nagram/chats/local_pins_model.cpp
    nagram/chats/cleanup.cpp
    nagram/chats/cleanup_model.cpp
    nagram/chats/reading_position.cpp
    nagram/chats/tools.cpp
    nagram/chats/community.cpp
    nagram/chats/folders.cpp
    nagram/chats/managed_folders.cpp
    nagram/interface/roundness.cpp
    nagram/settings/interface.cpp
    nagram/settings/restart.cpp
    nagram/menu/actions.cpp
    nagram/menu/batch.cpp
    nagram/menu/media.cpp
    nagram/menu/download.cpp
    nagram/menu/draft.cpp
    nagram/menu/rating.cpp
    nagram/menu/message_tools.cpp
    nagram/menu/model.cpp
    nagram/menu/repeat.cpp
    nagram/menu/reading.cpp
    nagram/menu/selection.cpp
	nagram/media/sticker_catalog.cpp
    nagram/media/audio.cpp
    nagram/media/cover.cpp
    nagram/media/sticker_export.cpp
    nagram/media/sticker_export_model.cpp
    nagram/media/url_template.cpp
    nagram/media/extras.cpp
    nagram/media/local_faved.cpp
    nagram/media/local_faved_model.cpp
    nagram/messages/format.cpp
    nagram/messages/markdown.cpp
    nagram/messages/content.cpp
    nagram/messages/badges.cpp
    nagram/messages/effects.cpp
    nagram/messages/online.cpp
    nagram/messages/reactions.cpp
    nagram/messages/reading.cpp
    nagram/tests/menu_scenario.cpp
    nagram/filters/model.cpp
    nagram/filters/view.cpp
    nagram/filters/settings.cpp
    nagram/filters/menu.cpp
    nagram/filters/hidden_messages.cpp
    nagram/links/model.cpp
    nagram/links/open.cpp
    nagram/links/behavior.cpp
    nagram/links/inline_bot.cpp
    nagram/links/inline_rules.cpp
    nagram/links/inline_settings.cpp
    nagram/links/webview.cpp
    nagram/links/settings.cpp
    nagram/network/model.cpp
    nagram/network/runtime.cpp
    nagram/notifications/model.cpp
    nagram/notifications/review.cpp
    nagram/notifications/settings.cpp
    nagram/snapshot/snapshot.cpp
    nagram/snapshot/cloud_theme.cpp
    nagram/snapshot/cloud_theme_model.cpp
    nagram/privacy/profile.cpp
    nagram/privacy/registration_model.cpp
    nagram/privacy/alias.cpp
    nagram/privacy/admin_shortcuts.cpp
    nagram/privacy/display.cpp
    nagram/privacy/alias_model.cpp
    nagram/privacy/protection.cpp
    nagram/privacy/protection_model.cpp
    nagram/settings/home.cpp
    nagram/settings/rules.cpp
    nagram/settings/network.cpp
    nagram/settings/chats.cpp
    nagram/settings/compose.cpp
	nagram/settings/config.cpp
    nagram/settings/cloud_sync.cpp
    nagram/sync/model.cpp
    nagram/sync/saved_messages.cpp
    nagram/sync/service.cpp
    nagram/settings/media.cpp
    nagram/settings/menu.cpp
    nagram/settings/privacy.cpp
    nagram/settings/messages.cpp
    nagram/settings/services.cpp
    nagram/services/credentials.cpp
    nagram/services/auto_translate.cpp
    nagram/services/auto_translate_model.cpp
    nagram/services/chat_translation.cpp
    nagram/services/chat_translation_model.cpp
    nagram/services/context.cpp
    nagram/services/context_model.cpp
    nagram/services/model.cpp
    nagram/services/presets.cpp
    nagram/services/store.cpp
    nagram/services/summary.cpp
    nagram/services/summary_model.cpp
    nagram/services/request.cpp
    nagram/services/translation.cpp
    nagram/services/draft_translation.cpp
    nagram/services/send_translation.cpp
    nagram/services/send_translation_model.cpp
    nagram/services/transcription.cpp
    nagram/services/transcription_queue.cpp
    nagram/services/system_ai.cpp
)

if (nagram_sources)
    nice_target_sources(Telegram ${src_loc} PRIVATE ${nagram_sources})
endif()

include(${CMAKE_CURRENT_LIST_DIR}/nagram_version.cmake)

if (NOT DESKTOP_APP_DISABLE_AUTOUPDATE)
    # The packer signs only v2 packages and needs no upstream private keys.
    # Its target is created after this file is included.
    cmake_language(DEFER CALL
        target_compile_definitions Packer PRIVATE PACKER_DISABLE_PRIVATE)
endif()

if (APPLE AND NOT DESKTOP_APP_DISABLE_SWIFT6)
    enable_language(Swift)
    set_target_properties(Telegram PROPERTIES LINKER_LANGUAGE CXX)
    set(nagram_swift_deployment "${CMAKE_OSX_DEPLOYMENT_TARGET}")
    if (NOT nagram_swift_deployment OR nagram_swift_deployment VERSION_LESS 11.0)
        set(nagram_swift_deployment 11.0)
    endif()
    if (NOT CMAKE_GENERATOR STREQUAL "Xcode")
        set(nagram_swift_arch "${CMAKE_OSX_ARCHITECTURES}")
        if (NOT nagram_swift_arch)
            set(nagram_swift_arch "${CMAKE_SYSTEM_PROCESSOR}")
        endif()
        list(LENGTH nagram_swift_arch nagram_swift_arch_count)
        if (NOT nagram_swift_arch_count EQUAL 1)
            message(FATAL_ERROR "Use Xcode for a universal Swift build, or select one CMAKE_OSX_ARCHITECTURES value.")
        endif()
    endif()
    function(nagram_configure_swift_target target_name)
        if (CMAKE_GENERATOR STREQUAL "Xcode")
            set_target_properties(${target_name} PROPERTIES
                XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "${nagram_swift_deployment}")
        else()
            target_compile_options(${target_name} PRIVATE
                "-target" "${nagram_swift_arch}-apple-macos${nagram_swift_deployment}")
        endif()
    endfunction()
    nagram_configure_swift_target(lib_translate)
    find_library(NAGRAM_FOUNDATION_MODELS FoundationModels)
    if (NAGRAM_FOUNDATION_MODELS)
        add_library(nagram_system_ai STATIC
            ${src_loc}/nagram/services/system_ai.swift)
        set_target_properties(nagram_system_ai PROPERTIES
            Swift_LANGUAGE_VERSION 6
            Swift_COMPILATION_MODE wholemodule)
        nagram_configure_swift_target(nagram_system_ai)
        target_link_options(nagram_system_ai INTERFACE
            "SHELL:-weak_framework FoundationModels")
        target_link_libraries(Telegram PRIVATE nagram_system_ai)
        target_compile_definitions(Telegram PRIVATE NAGRAM_SYSTEM_AI)
    endif()
endif()

nice_target_sources(Telegram ${res_loc} PRIVATE qrc/nagram.qrc)
if (APPLE)
    nice_target_sources(Telegram ${res_loc} PRIVATE qrc/nagram_mac.qrc)
endif()

# Compiles the Icon Composer document so macOS renders the app icon itself.
function(nagram_mac_app_icon result)
    set(${result} FALSE PARENT_SCOPE)
    execute_process(
        COMMAND xcodebuild -version
        OUTPUT_VARIABLE xcode_version_output
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    if (NOT xcode_version_output MATCHES "Xcode ([0-9]+)")
        message(STATUS "Nagram: Xcode not found, using the static app icon.")
        return()
    elseif (CMAKE_MATCH_1 LESS 26)
        message(STATUS "Nagram: Xcode ${CMAKE_MATCH_1} cannot compile .icon documents, using the static app icon.")
        return()
    endif()
    set(icon_source ${res_loc}/branding/Nagram.icon)
    set(icon_output ${CMAKE_CURRENT_BINARY_DIR}/nagram_icon)
    set(icon_deployment "${CMAKE_OSX_DEPLOYMENT_TARGET}")
    if (NOT icon_deployment)
        set(icon_deployment 11.0)
    endif()
    file(GLOB_RECURSE icon_inputs CONFIGURE_DEPENDS ${icon_source}/*)
    add_custom_command(
        OUTPUT ${icon_output}/Assets.car ${icon_output}/Nagram.icns
        COMMAND ${CMAKE_COMMAND} -E make_directory ${icon_output}
        COMMAND xcrun actool ${icon_source}
            --compile ${icon_output}
            --app-icon Nagram
            --platform macosx
            --minimum-deployment-target ${icon_deployment}
            --output-partial-info-plist ${icon_output}/partial.plist
            --output-format human-readable-text
        DEPENDS ${icon_inputs}
        VERBATIM)
    set_source_files_properties(
        ${icon_output}/Assets.car
        ${icon_output}/Nagram.icns
        PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    target_add_resource(Telegram
        ${icon_output}/Assets.car
        ${icon_output}/Nagram.icns)
    set(${result} TRUE PARENT_SCOPE)
endfunction()

if (DESKTOP_APP_TEST_APPS)
    add_executable(test_nagram)
    init_target(test_nagram "(tests)")

    nice_target_sources(test_nagram ${src_loc} PRIVATE
        nagram/tests/test_lang.cpp
        nagram/tests/test_options.cpp
        nagram/tests/test_spacing.cpp
        nagram/tests/test_services.cpp
        nagram/tests/test_chat_translation.cpp
        nagram/tests/test_auto_translate.cpp
        nagram/tests/test_filters.cpp
        nagram/tests/test_filter_scopes.cpp
        nagram/tests/test_links.cpp
        nagram/tests/test_inline_rules.cpp
        nagram/tests/test_privacy.cpp
        nagram/tests/test_p3_misc.cpp
        nagram/tests/test_local_lists.cpp
        nagram/tests/test_network.cpp
        nagram/tests/test_media.cpp
        nagram/tests/test_sync.cpp
        nagram/tests/test_export.cpp
        nagram/tests/test_markdown.cpp
        nagram/tests/test_cleanup.cpp
        nagram/tests/test_notifications.cpp
        nagram/tests/test_send_translation.cpp
        nagram/compose/spacing.cpp
        nagram/core/exchange.cpp
        nagram/core/regex.cpp
        nagram/export/range_model.cpp
        nagram/interface/main_menu_model.cpp
        nagram/menu/model.cpp
        nagram/messages/markdown.cpp
        nagram/services/auto_translate_model.cpp
        nagram/services/chat_translation_model.cpp
        nagram/services/context_model.cpp
        nagram/services/model.cpp
        nagram/services/presets.cpp
        nagram/services/summary_model.cpp
        nagram/services/send_translation_model.cpp
        nagram/services/transcription_queue.cpp
        nagram/filters/model.cpp
        nagram/links/model.cpp
        nagram/links/inline_rules.cpp
        nagram/links/webview.cpp
        nagram/privacy/protection_model.cpp
        nagram/privacy/registration_model.cpp
        nagram/chats/local_pins_model.cpp
        nagram/chats/cleanup_model.cpp
        nagram/media/local_faved_model.cpp
        nagram/network/model.cpp
        nagram/notifications/model.cpp
        nagram/media/url_template.cpp
        nagram/media/sticker_export_model.cpp
        nagram/snapshot/cloud_theme_model.cpp
        nagram/sync/model.cpp
    )

    target_include_directories(test_nagram PRIVATE
        ${src_loc}
        ${CMAKE_CURRENT_SOURCE_DIR}/lib_ui)

    target_link_libraries(test_nagram PRIVATE
        desktop-app::lib_base
        desktop-app::lib_crl
        desktop-app::external_qt
    )

    target_compile_definitions(test_nagram PRIVATE
        NAGRAM_LANG_SOURCE_DIR="${res_loc}/langs"
    )

    set_target_properties(test_nagram PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/nagram-tests/$<CONFIG>"
    )

    add_dependencies(Telegram test_nagram)
endif()

# The upstream test harness copies not_null<QAction*> in two range loops. GCC
# reports that and the Linux Debug build of CI turns warnings into errors.
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set_source_files_properties(
        ${CMAKE_CURRENT_SOURCE_DIR}/SourceFiles/test/test_menu.cpp
    PROPERTIES
        COMPILE_OPTIONS -Wno-error=range-loop-construct
    )
endif()
