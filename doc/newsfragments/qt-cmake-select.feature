Allow Qt version to be selected via CMake

CMake now uses a sane default of Qt 5, but allows for a CMake option of
`QT_DEFAULT_MAJOR_VERSION`, for overriding of the Qt library used during build.

For example, setting `QT_DEFAULT_MAJOR_VERSION` to `6` uses Qt 6, and setting to
`5` uses Qt 5. Older versions are not supported.
