cmake_minimum_required(VERSION 3.16)
project(Duplicate-File-Finder VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wpedantic")

# Dependencies
find_package(OpenSSL REQUIRED COMPONENTS Crypto)
find_package(Boost REQUIRED COMPONENTS program_options)

# Main target
add_executable(dupfind main.cpp)
target_link_libraries(dupfind PRIVATE OpenSSL::Crypto Boost::program_options)

# Installation
include(GNUInstallDirs)
install(TARGETS dupfind RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Packaging (Debian .DEB and .TAR.GZ archive)
include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README  "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_DESCRIPTION "Command line utility that recursively scans \
a directory for duplicate files.")
set(CPACK_GENERATOR "DEB;TGZ")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS On)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Pieter P")
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_ALL_IN_ONE_PACKAGE ON)
install(FILES
  ${CPACK_RESOURCE_FILE_README}
  ${CPACK_RESOURCE_FILE_LICENSE}
  DESTINATION .
  COMPONENT Metadata
  EXCLUDE_FROM_ALL)
include(CPack)
