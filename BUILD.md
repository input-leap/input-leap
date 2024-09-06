# The Basics

You want to build input-leap, but don't know where to start. This project uses the cmake build system and includes a handy script for performing a clean build (clean\_build.sh or clean\_build.bat). You'll want to use this script to build this project since it ensures that your build isn't stale as you're reconfiguring the project.

# System Dependencies

When first trying to build this project, you're likely to encounter missing dependencies. These dependencies will be reported as an identifier such as `xrandr`, but the missing dependency that cmake is searching for would be `libxrandr-dev` or `libxrandr-devel`.

# Build Settings

You may need to alter build settings to achieve your desired configuration. You can create build\_env.sh to configure build options such as INPUTLEAP\_BUILD\_LIBEI by adding them to B\_CMAKE\_FLAGS.
