# Nagram Desktop: a release is <upstream version>.<revision>. The revision and
# the channel of the latest release are kept in build/nagram_version, so the
# upstream version files stay as upstream wrote them.

set(nagram_version_file ${CMAKE_CURRENT_SOURCE_DIR}/build/nagram_version)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${nagram_version_file})

set(nagram_revision "")
set(nagram_channel "")
file(STRINGS ${nagram_version_file} nagram_version_lines)
foreach (line ${nagram_version_lines})
    if (line MATCHES "^NagramRevision +([1-9][0-9]?[0-9]?)$" AND nagram_revision STREQUAL "")
        set(nagram_revision ${CMAKE_MATCH_1})
    elseif (line MATCHES "^NagramChannel +(stable|beta)$" AND nagram_channel STREQUAL "")
        set(nagram_channel ${CMAKE_MATCH_1})
    else()
        message(FATAL_ERROR "Bad line in ${nagram_version_file}: ${line}")
    endif()
endforeach()
if (nagram_revision STREQUAL "" OR nagram_channel STREQUAL "")
    message(FATAL_ERROR "${nagram_version_file} must set NagramRevision and NagramChannel.")
endif()

set(nagram_version_string ${desktop_app_version_string}.${nagram_revision})
if (nagram_channel STREQUAL "beta")
    set(nagram_version_beta true)
else()
    set(nagram_version_beta false)
endif()

# A build of the default branch is told from the release it follows by its
# build number, tools/nagram/release_version.py --ci-build; a release has 0.
set(NAGRAM_CI_BUILD 0 CACHE STRING "Build number of a build of the default branch.")
if (NOT NAGRAM_CI_BUILD MATCHES "^(0|[1-9][0-9]*)$" OR NAGRAM_CI_BUILD GREATER_EQUAL 4194304)
    message(FATAL_ERROR "NAGRAM_CI_BUILD must be a number below 4194304: ${NAGRAM_CI_BUILD}")
endif()
message(STATUS "Nagram: version ${nagram_version_string} (${nagram_channel}), build ${NAGRAM_CI_BUILD}.")

# A generated header keeps a revision bump from recompiling the whole target.
set(nagram_version_gen ${CMAKE_CURRENT_BINARY_DIR}/gen)
file(CONFIGURE
    OUTPUT ${nagram_version_gen}/nagram_version_data.h
    CONTENT "// Generated from Telegram/build/nagram_version.
#pragma once

namespace Nagram {

inline constexpr auto kVersionStr = \"@nagram_version_string@\";
inline constexpr auto kVersionRevision = @nagram_revision@;
inline constexpr auto kVersionBuild = @NAGRAM_CI_BUILD@;
inline constexpr auto kVersionBeta = @nagram_version_beta@;

} // namespace Nagram
"
    @ONLY)
target_include_directories(Telegram PRIVATE ${nagram_version_gen})
