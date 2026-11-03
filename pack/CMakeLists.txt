cmake_minimum_required(VERSION 3.15)
project(GrnGamePackaging)

install(DIRECTORY "${STAGING_DIR}/" DESTINATION ".")

set(CPACK_PACKAGE_NAME "GrnGame")
set(CPACK_PACKAGE_VERSION "${GRNGAME_VERSION}")
set(CPACK_PACKAGE_VENDOR "GrnGame")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A simple 2D pixel art platformer engine.")

if(WIN32)
    set(CPACK_GENERATOR "WIX;ZIP")
elseif(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "DEB;RPM;TGZ")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Baptiste Guerin baptiste.guerin34@gmail.com")
    set(CPACK_RPM_PACKAGE_AUTOREQPROV "no")
endif()

include(CPack)