//! Crate for the Input Leap's Rust core.
#![deny(
    warnings,
    missing_copy_implementations,
    missing_debug_implementations,
//    missing_docs,
    clippy::all,
    clippy::cargo,
    trivial_casts,
    trivial_numeric_casts,
    unused_import_braces,
    unused_qualifications,
    unused_extern_crates,
    variant_size_differences
)]
#![allow(non_upper_case_globals, non_snake_case)]

use const_format::concatcp;

const kApplication: &'static str = "Input Leap";

const kCopyright: &'static str = "
    Copyright (C) 2018 The InputLeap Developers\n
    Copyright (C) 2012-2016 Symless Ltd\n
    Copyright (C) 2008-2014 Nick Bolton\n
    Copyright (C) 2002-2014 Chris Schoeneman";

const kContact: &'static str = "Email: todo@mail.com";

const kWebsite: &'static str = "https://github.com/input-leap/input-leap";

const kVersion: &'static str = env!("CARGO_PKG_VERSION");

const kAppVersion: &'static str = concatcp!("InputLeap ", kVersion);

#[cxx::bridge]
mod ffi {
    extern "Rust" {
        fn get_kApplication() -> &'static str;
        fn get_kCopyright() -> &'static str;
        fn get_kContact() -> &'static str;
        fn get_kWebsite() -> &'static str;
        fn get_kVersion() -> &'static str;
        fn get_kAppVersion() -> &'static str;
    }
}

fn get_kApplication() -> &'static str {
    kApplication
}

fn get_kCopyright() -> &'static str {
    kCopyright
}

fn get_kContact() -> &'static str {
    kContact
}

fn get_kWebsite() -> &'static str {
    kWebsite
}

fn get_kVersion() -> &'static str {
    kVersion
}

fn get_kAppVersion() -> &'static str {
    kAppVersion
}
