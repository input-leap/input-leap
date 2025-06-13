Release notes
=============

[comment]: <> (towncrier release notes start)

InputLeap `3.0.3` ( `2025-06-13` )
================================

=====================================================================

Bug fixes
---------

- Fixed cursor jumping to center of primary display when entering macOS screen by moving isOnScreen flag
- Flatpak appid has been changed to `io.github.input_leap.input-leap` to resolve to Github repository correctly.
- Fixed a segmentation fault that would occur when configuring servers on certain platforms. This solution was proposed by sithlord48 and they used it to fix the issue for deskflow.
- EI screen shape is now properly reset on update, shape x and y are not bound to 0 0 anymore.
- Fixed missing keymap configs on wayland as the XKB keymap from Ei was not properly loading. The respective keymap file was not seeked to the start on some systems.
- Fixed a problem with UTF-16 surrogate pairs that caused broken characters (especially emojis) when copying to the clipboard from a Windows machine
- Fixed input capture Disabled signal, Enable method is now properly called after Disable.
- Fixed a long standing bug with modifiers and dead keys on Windows because their state was not being reset.
- Fixed an issue with mapping keys in OSX clients introduced in #1635, where keys like the equals sign "=" or the back tick "`" wouldn't be mapped correctly.
- Remove check requiring the Alt modifier for non-extended VK_SNAPSHOT scancode in Win32.
  Check is irrelevant for modern keyboards, and breaks Print Screen functionality.

Features
--------

- Add a reload action to the tray icon menu.
  Sometimes the clipboard copy function does not work. In this case, when the input-leap is reloaded, the function works normally again.
  To make reload easier, add a reload action to the tray icon.

InputLeap `3.0.2` ( `2024-10-12` )
==================================

Bug fixes
---------

- Fixed broken screen name entry in screen settings dialog.
- Released deb packages support Wayland on Ubuntu 24.04 and 24.10.

Features
--------

- Added a Debian package for Ubuntu 24.04 and 24.10 to the set of released packages.

Deprecations and Removals
-------------------------

- Minimum MacOS version has been increased to 10.15.
- Packages for Ubuntu 20.04 are no longer distributed.

Please open a Github issue you're still using these versions and can't upgrade.  Depending on
popular demand removal of old OS support may be reverted.


InputLeap `3.0.1` ( `2024-10-10` )
==================================

Bug fixes
---------

- Added automatic user config migration from old config location on Linux.
- Fixed generation of Windows, Ubuntu, Debian and flatpak installers.


InputLeap `3.0.0` ( `2024-10-04` )
==================================

Features
--------

- Added Wayland support. Note that XWayland won't work properly and warning is printed.
- Create an `uninstall` target for Makefiles

  This allows for developers (or users who are testing) to remove Input Leap from
  their system, based on `$PREFIX`.
- Added a new clipboard sharing size limiter to prevent transferring clipboards past a default
  value in bytes.
- Allow Qt version to be selected via CMake

  CMake now uses a sane default of Qt 5, but allows for a CMake option of
  `QT_DEFAULT_MAJOR_VERSION`, for overriding of the Qt library used during build.

  For example, setting `QT_DEFAULT_MAJOR_VERSION` to `6` uses Qt 6, and setting to
  `5` uses Qt 5. Older versions are not supported.
- Added new converters for X11 to support copy pasting png/tiff/jpg/webp formats.
- Added support for building against Qt 6 in addition to Qt 5.

Bug fixes
---------

- Corrected macOS packaging to provide a better error message when a user attempts to launch
  InputLeap on an incompatible macOS version. (https://github.com/input-leap/input-leap/issues/1260).
- Removed limitation in Corner Size GUI to allow higher value than 99
  (https://github.com/input-leap/input-leap/issues/159)
- Fixed a dead lock entered when a server screen is suspended, so that screen-resume message will
  work as expected.
- Fixed spacing for max clipboard input on in the advanced server settings
  (https://github.com/input-leap/input-leap/issues/1644).
- Added a log limit to 10,000 lines to prevent RAM use from constantly increasing.
- Reduced CPU usage of log window.
- Fixed clipboard sharing toggle in GUI (https://github.com/input-leap/input-leap/issues/1789)
- Fixed generation of new certificates.
- The source distribution tarball no longer contains ``debian`` folder which previously clashed
  with data provided by the Debian packaging.
- Fixed support for debugging commpand line apps on Linux. Previously they would exit whenever debugger
  tries to interrupt them which makes debugging useless.
- Fixed out of bounds write which sometimes causes crash when switching screens.
- Fixed per-display scaling on Windows (https://github.com/input-leap/input-leap/issues/1952,
  https://github.com/input-leap/input-leap/issues/94, https://github.com/input-leap/input-leap/issues/206)
- Fixed wrong encoding for text copied between Linux and Windows
  (https://github.com/input-leap/input-leap/issues/1037,
  https://github.com/input-leap/input-leap/issues/1137).
- Fixed issue with X11 clipboard sharing images as text targets or in other targets than the one it
  should have.
- Fixed build on GCC 11.2 or newer (https://github.com/input-leap/input-leap/issues/1366).
- Fixed potential crash during app shutdown.
- Updated Japanese translation.
- Fixed "Fix preserve Focus" option on Linux servers (https://github.com/input-leap/input-leap/issues/1066).
- Fixed a potential memory leak that may cause server memory usage to grow during repeated reconnection attempts.
- Renamed references to InputLeap in translations.
- Fixed InputLeap code to produce reproducible build artifacts.
- Fixed potential crashes during server shutdown due to a stale pointer to freed memory.
- Updated Server IP shortcut to not conflict with Start/Stop button.
- Fixed bug that caused tilde (~) and grave (`) to stop being sendable.

Improved Documentation
----------------------

- Updated FAQs in project README.md with more detail on OS support and links to issues for infrequent users. (https://github.com/input-leap/input-leap/issues/1260).
- Added a note in README.md on how to obtain test builds.
- Fixed `input-leap.conf.example-advanced` to accurately include the advanced configuration described in/intended by the comment within it.
- Update LICENSE file in repository to reflect fork from Barrier organisation.
- Fixed FAQ link to Linux drag and drop issue.
- Minor grammatical updates in newsfragments README.md.

Changes
-------

- Changed server configuration dialog initial tab to "Screens".
- Changed user configuration location on Linux from ~/.local/share to ~/.config.

Deprecations and Removals
-------------------------

- The --no-xinitthreads commandline option has been deprecated and no longer has any effect.


Barrier `2.4.0` ( `2021-11-01` )
================================

Security fixes
--------------

- Barrier now supports client identity verification (fixes CVE-2021-42072, CVE-2021-42073).

  Previously a malicious client could connect to Barrier server without any authentication and
  send application-level messages. This made the attack surface of Barrier significantly larger.
  Additionally, in case the malicious client got possession of a valid screen name by brute forcing
  or other means it could modify the clipboard contents of the server.

  To support seamless upgrades from older versions of Barrier this is currently disabled by default.
  The feature can be enabled in the settings dialog. If enabled, older clients of Barrier will be
  rejected.

- Barrier now uses SHA256 fingerprints for establishing security of encrypted SSL connections.
  After upgrading client to new version the existing server fingerprint will need to be approved
  again. Client and server will show both SHA1 and SHA256 server fingerprints to allow
  interoperability with older versions of Barrier.

All of the above security issues have been reported by Matthias Gerstner who was really helpful
resolving them.

Bug fixes
---------

- Fixed build failure on mips*el and riscv64 architecture.
- Fixed reading of configuration on Windows when the paths contain non-ASCII characters
(https://github.com/input-leap/input-leap/issues/976, https://github.com/input-leap/input-leap/issues/974,
 https://github.com/input-leap/input-leap/issues/444).
- Barrier no longer uses openssl CLI tool for any operations and hooks into the openssl library directly.
- More X11 clipboard MIME types have been mapped to corresponding converters (https://github.com/input-leap/input-leap/issues/344).
- Fixed setup of multiple actions associated with a hotkey.
- Fixed setup of hotkeys with special characters such as comma and semicolon
  (https://github.com/input-leap/input-leap/issues/778).
- Fixed transfer of non-ASCII characters coming from a Windows server in certain cases
 (https://github.com/input-leap/input-leap/issues/527).
- Barrier will now regenerate server certificate if it's invalid instead of failing to launch
 (https://github.com/input-leap/input-leap/issues/802)
- Added support for additional keys on Sun Microsystems USB keyboards
 (https://github.com/input-leap/input-leap/issues/784).
- Updated Chinese translation.
- Updated Slovak translation.
- Theme icons are now preferred to icons distributed together with Barrier
 (https://github.com/input-leap/input-leap/issues/471).
- Fixed incorrect setup of Barrier service path on Windows.

Features
--------

- Added `--drop-target` option that improves drag and drop support on Windows when Barrier is
  being run as a portable app.
- The `--enable-crypto` command line option has been made the default to reduce chances of
  accidental security mishaps when configuring Barrier from command line.
  A new `--disable-crypto` command line option has been added to explicitly disable encryption.
- Added support for randomart images for easier comparison of SSL certificate fingerprints.
  The algorithm is identical to what OpenSSH uses.
- Implemented a configuration option for Server GUI auto-start.
- Made it possible to use keyboard instead of mouse to modify screen layout.
- Added support for keyboard backlight media keys
- Added support for Eisu_toggle and Muhenkan keys
- Added `--profile-dir` option that allows to select custom profile directory.

Barrier `2.3.4` ( `2021-11-01` )
================================

Security fixes
--------------

- Barrier will now correctly close connections when the app-level handshake fails (fixes CVE-2021-42075).

  Previously repeated failing connections would leak file descriptors leading to Barrier being unable
  to receive new connections from clients.

- Barrier will now enforce a maximum length of input messages (fixes CVE-2021-42076).

  Previously it was possible for a malicious client or server to send excessive length messages
  leading to denial of service by resource exhaustion.

- Fixed a bug which caused Barrier to crash when disconnecting a TCP session just after sending
  Hello message (fixes CVE-2021-42074).
  This bug allowed an unauthenticated attacker to crash Barrier with only network access.

All of the above security issues have been reported by Matthias Gerstner who was really helpful
resolving them.

Bug fixes
---------

- Fixed a bug in SSL implementation that caused invalid data occasionally being sent to clients
  under heavy load.
