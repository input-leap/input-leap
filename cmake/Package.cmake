if (UNIX)
    set(CPACK_PACKAGE_NAME "input-leap")
    set(CPACK_PACKAGE_FILE_NAME
        "input-leap_${INPUTLEAP_VERSION_MAJOR}.${INPUTLEAP_VERSION_MINOR}.${INPUTLEAP_VERSION_PATCH}_amd64")
    set(CPACK_PACKAGE_VERSION_MAJOR ${INPUTLEAP_VERSION_MAJOR})
    set(CPACK_PACKAGE_VERSION_MINOR ${INPUTLEAP_VERSION_MINOR})
    set(CPACK_PACKAGE_VERSION_PATCH ${INPUTLEAP_VERSION_PATCH})
    set(CPACK_GENERATOR "DEB;TBZ2")
    set(CPACK_SOURCE_GENERATOR "TXZ")
    set(CPACK_SOURCE_IGNORE_FILES "/build/;\.gitignore$;/\.git/;/.github/;\.swp$;build_env\.*")

    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "InputLeap contributors")
    set(CPACK_DEBIAN_PACKAGE_SECTION "net")
    set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/input-leap/input-leap")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS
        "libssl3 (>= 1.1.1), libx11-6, libxext6, libxrandr2, libxinerama1, libxtst6, libavahi-compat-libdnssd1, libei1 (>= 1.0), libportal1")
    set(CPACK_DEBIAN_PACKAGE_REPLACES "input-leap")
    set(CPACK_DEBIAN_PACKAGE_CONFLICTS "input-leap")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

    # Maintainer scripts: kill running processes before install/remove.
    # preinst also purges broken residual installations on a fresh install.
    # On upgrade, apt/dpkg resolve the conflict via Replaces+Conflicts above,
    # so the old prerm runs first (stopping processes) before our preinst.
    set(DEBIAN_PREINST "${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian/preinst")
    set(DEBIAN_PRERM   "${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian/prerm")
    execute_process(COMMAND chmod +x "${DEBIAN_PREINST}" "${DEBIAN_PRERM}")
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${DEBIAN_PREINST};${DEBIAN_PRERM}")

    include(CPack)
endif()
