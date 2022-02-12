fn main() {
    _ = cxx_build::bridge("src/lib.rs")
        .file("../lib/rustlib/rust.cpp");

    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=../lib/rustlib/rust.cpp");
}
