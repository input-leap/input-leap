Release notes
=============

[comment]: <> (towncrier release notes start)

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

Bug fixes
---------

- Fixed build failure on mips*el and riscv64 architecture.
- Fixed reading of configuration on Windows when the paths contain non-ASCII characters
(https://github.com/debauchee/barrier/issues/976, https://github.com/debauchee/barrier/issues/974,
 https://github.com/debauchee/barrier/issues/444).
- Barrier no longer uses openssl CLI tool for any operations and hooks into the openssl library directly.
- More X11 clipboard MIME types have been mapped to corresponding converters (https://github.com/debauchee/barrier/issues/344).
- Fixed setup of multiple actions associated with a hotkey.
- Fixed setup of hotkeys with special characters such as comma and semicolon
  (https://github.com/debauchee/barrier/issues/778).
- Fixed transfer of non-ASCII characters coming from a Windows server in certain cases
 (https://github.com/debauchee/barrier/issues/527).
- Barrier will now regenerate server certificate if it's invalid instead of failing to launch
 (https://github.com/debauchee/barrier/issues/802)
- Added support for additional keys on Sun Microsystems USB keyboards
 (https://github.com/debauchee/barrier/issues/784).
- Updated Chinese translation.
- Updated Slovak translation.
- Theme icons are now preferred to icons distributed together with Barrier
 (https://github.com/debauchee/barrier/issues/471).
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

- Fixed a bug which caused Barrier to crash when disconnecting a TCP session just after sending Hello message.
  This bug allowed an unauthenticated attacker to crash Barrier with only network access.

All of the above security issues have been reported by Matthias Gerstner who was really helpful
resolving them.

Bug fixes
---------

- Fixed a bug in SSL implementation that caused invalid data occasionally being sent to clients
  under heavy load.
