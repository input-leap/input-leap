#!/bin/sh

# $1 = binary (program or dylib)
B_TARGET=$1
if [ "x$B_TARGET" = "x" ]; then

    # add warning for users running manually
    warn() { tput bold; tput setaf 3; echo "$@"; tput sgr0 ; }
    warn "The scripts build_installer.sh and reref_dylibs.sh have been deprecated."
    warn "Please use build_dist.sh instead to deploy using macdeployqt"

    echo "Which binary needs to be re-referenced?"
    exit 1
fi

cd "$(dirname "$B_TARGET")" || exit 1

# where to find non-system libraries relative to target's directory.
# the vast majority of the time this should be empty
B_REL_PATH=$2

# we're in target's directory now. trim off the path
B_TARGET=$(basename "$B_TARGET")

# get a list of non-system libraries and make local copies
B_LIBS=$(otool -XL "$B_TARGET" | awk '{ print $1 }' | grep -Ev '^(/usr/lib|/System)')
[ $? -ne 0 ] && exit 1
for B_LIB in $B_LIBS; do
    B_LIB_NAME=$(basename "$B_LIB")
    # otool reports input-leap as "input-leap:" which fails self-reference test below
    B_LIB_NAME=${B_LIB_NAME//:}

    # ignore self-references
    [ "$B_TARGET" = "$B_LIB_NAME" ] && continue

    B_DST=${B_REL_PATH}${B_LIB_NAME}
    if [ ! -e "$B_DST" ]; then
        cp "$B_LIB" "$B_DST" || exit 1
        chmod u+rw "$B_DST" || exit 1
        # recursively call this script on libraries purposefully not passing
        # $B_REL_PATH so that it is only used explicitly
        $0 "$B_DST"
    fi

    # adjust the target's metadata to point to the local copy
    # rather than the system-wide copy which would only exist on
    # a development machine
    install_name_tool -change "$B_LIB" "@loader_path/$B_DST" "$B_TARGET" || exit 1
done
