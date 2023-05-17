# Input Leap

<!-- add CI badges, flatpak, snap, use github actions badges or Azure? Decide on
*one* -->

### Contact info:

- `#inputleap` on LiberaChat IRC network, for support with Input Leap.
- `#inputleap-dev` on LiberaChat IRC network, for discussing development of Input Leap.

### What is it?

Input Leap is software that mimics the functionality of a KVM switch, which
historically would allow you to use a single keyboard and mouse to control
multiple computers by physically turning a dial on the box to switch the machine
you're controlling at any given moment. Input Leap does this in software, allowing
you to tell it which machine to control by moving your mouse to the edge of the
screen, or by using a keypress to switch focus to a different system.

<!-- add info about fork from barrier
Input Leap was forked from Symless's Synergy 1.9 codebase. Synergy was a
commercialized reimplementation of the original CosmoSynergy written by Chris
Schoeneman.  -->

At the moment, Input Leap is not compatible with Synergy.

Input Leap needs to be installed on all machines that will share keyboard and
mouse.

### What's different?

Whereas Synergy has moved beyond its goals from the 1.x era, Input Leap aims to
maintain that simplicity. Input Leap will let you use your keyboard and mouse
from one computer to control one or more other computers.

Clipboard sharing is supported.

That's it.

### Project goals

Hassle-free reliability. We are users, too. Input Leap was created so that we could
solve the issues we had with Synergy and then share these fixes with other
users.

Compatibility. We use more than one operating system and you probably do, too.
Windows, macOS, Linux, FreeBSD... Input Leap should "just work". We will also
have our eye on Wayland when the time comes.

Communication. Everything we do is in the open. Our issue tracker will let you
see if others are having the same problem you're having and will allow you to
add additional information. You will also be able to see when progress is made
and how the issue gets resolved.

### Usage

1. Install and run Input Leap on each machine that will be sharing.
2. On the machine with the keyboard and mouse, make it the server.
3. Click the "Configure server" button and drag a new screen onto the grid for
    each client machine.
4. Ensure the "screen name" matches exactly (case-sensitive) for each configured
    screen -- the clients' Input Leap windows will tell you their screen names
    (just above the server IP).
5. On the client(s), put in the server machine's IP address (or use Bonjour/auto
    configuration when prompted) and "start" them.

You should see `Input Leap is running` on both server and clients.

You should now be able to move the mouse between all the screens as if they were
the same machine.

Note that if the keyboard's Scroll Lock is active then this will prevent the
mouse from switching screens.

To find information about configuring Input Leap look at the
[wiki](https://github.com/input-leap/input-leap/wiki)

### Contact & support

Please be aware that the *only* way to draw our attention to a bug is to create
a new issue in [the issue tracker](https://github.com/input-leap/input-leap/issues).

Always follow the template, it's there for a reason. Failure to do so will
inevitably result in a delay in the issue being fixed!

Please also see the IRC channels at the top of the README for other methods of
support.

### Contributions

At this time we are looking for developers to help fix the issues found in the
issue tracker.

Submit pull requests once you've polished up your patch and we'll review and
possibly merge it.

Most pull requests will need to include a release note.

See `docs/newsfragments/README.md` for documentation of how to do that.

## Distro specific packages

While not a comprehensive list, [repology](https://repology.org/project/input-leap/versions) provides a decent list of distro
specific packages.

[![Packaging status](https://repology.org/badge/vertical-allrepos/input-leap.svg)](https://repology.org/project/input-leap/versions)

## FAQ - Frequently Asked Questions

**Q: Input Leap vs. barrier?**

> A: Input Leap is a fork of barrier, by **barrier's active maintainers**. See [issue #1414](https://github.com/input-leap/input-leap/issues/1414)
>    for more details. barrier is considered unmaintained.

**Q: Does drag and drop work on linux?**

> A: No *(see [#855](https://github.com/input-leap/input-leap/issues/855) if you'd like to change that)*


**Q: What OSes are supported?**

> A: The [most recent release](https://github.com/input-leap/input-leap/releases/latest) of Input Leap is known to work on:
>  - Windows 7, 8, 8.1, 10, and 11
>  - macOS *(previously known as OS X or Mac OS X)*
>    - _The current GUI does **not** work on OS versions prior to macOS 10.12 Sierra (but see the related answer below)_
>  - Linux
>  - FreeBSD
>  - OpenBSD


**Q: Are 32-bit versions of Windows supported?**

> A: Not officially.


__Q: Is it possible to use Input Leap on Mac OS X / OS X versions prior to 10.12?__

> A: Not officially.
>   - For OS X 10.10 Yosemite and later:
>     - [Input Leap v2.1.0](https://github.com/input-leap/input-leap/releases/tag/v2.1.0) or earlier _may_ work.
>   - For Mac OS X 10.9 Mavericks _(and perhaps earlier)_:
>     1. the command-line portions of the [current release](https://github.com/input-leap/input-leap/releases/latest) _should_ run fine.
>     2. The GUI will _not_ run, as that OS version does not include Apple's *Metal* framework.
>         - _(For a GUI workaround for Mac OS X 10.9, see the [discussion at issue #544](https://github.com/input-leap/input-leap/issues/544))_

> Note: Only versions [v2.3.4](https://github.com/input-leap/input-leap/releases/tag/v2.3.4) and [later](https://github.com/input-leap/input-leap/releases/latest) of Input Leap can be supported by this project.
>  - Anyone using an earlier version is advised to upgrade due to recently-addressed security vulnerabilities *(and other bug fixes)*.
>    - This is especially important for computers accessible from the public Internet *(or from other shared/untrusted networks, such as when using shared WiFi)*.


**Q: How do I load my configuration on startup?**

> A: Start the binary with the argument `--config <path_to_saved_configuration>`


**Q: After loading my configuration on the client the field 'Server IP' is still empty!**

> A: Edit your configuration to include the server's ip address manually with
>
>```
>(...)
>
>section: options
>    serverhostname=<AAA.BBB.CCC.DDD>
>```

**Q: Are there any other significant limitations with the current version of Input Leap?**

> A: Currently:
>    - Input Leap currently has limited UTF-8 support; issues have been reported with processing various languages.
>      - *(see [#860](https://github.com/input-leap/input-leap/issues/860))*
>    - There is interest in future support for the Wayland compositor/display server protocol *([official site](https://wayland.freedesktop.org/) | [Wikipedia article](https://en.wikipedia.org/wiki/Wayland_(display_server_protocol)))* on Linux.
>      - As of mid-2022, there is no expected completion date for *Wayland* support.
>      - *(see [#109](https://github.com/input-leap/input-leap/issues/109) and [#1251](https://github.com/input-leap/input-leap/issues/1251) for status or to volunteer your talents)*
>
> The complete list of open issues can be found in the ['Issues' tab on GitHub](https://github.com/input-leap/input-leap/issues?q=is%3Aissue+is%3Aopen). Help is always appreciated.
