# Barrier

Eliminate the barrier between your machines.
Find [releases for windows and macOS here](https://github.com/debauchee/barrier/releases).
Your distro probably already has barrier packaged for it, see [distro specific packages](#distro-specific-packages)
below for a list. Alternatively, we also provide a [flatpak](https://github.com/flathub/com.github.debauchee.barrier)
and a [snap](https://snapcraft.io/barrier).

### Contact info:

- `#barrier` on LiberaChat IRC network

#### CI Build Status

Master branch overall build status: [![Build Status](https://dev.azure.com/debauchee/Barrier/_apis/build/status/debauchee.barrier?branchName=master)](https://dev.azure.com/debauchee/Barrier/_build/latest?definitionId=1&branchName=master)

|Platform       |Build Status|
|            --:|:--         |
|Linux          |[![Build Status](https://dev.azure.com/debauchee/Barrier/_apis/build/status/debauchee.barrier?branchName=master&jobName=Linux%20Build)](https://dev.azure.com/debauchee/Barrier/_build/latest?definitionId=1&branchName=master)|
|Mac            |[![Build Status](https://dev.azure.com/debauchee/Barrier/_apis/build/status/debauchee.barrier?branchName=master&jobName=Mac%20Build)](https://dev.azure.com/debauchee/Barrier/_build/latest?definitionId=1&branchName=master)|
|Windows Debug  |[![Build Status](https://dev.azure.com/debauchee/Barrier/_apis/build/status/debauchee.barrier?branchName=master&jobName=Windows%20Build&configuration=Windows%20Build%20Debug)](https://dev.azure.com/debauchee/Barrier/_build/latest?definitionId=1&branchName=master)|
|Windows Release|[![Build Status](https://dev.azure.com/debauchee/Barrier/_apis/build/status/debauchee.barrier?branchName=master&jobName=Windows%20Build&configuration=Windows%20Build%20Release%20with%20Release%20Installer)](https://dev.azure.com/debauchee/Barrier/_build/latest?definitionId=1&branchName=master)|
|Snap           |[![Snap Status](https://build.snapcraft.io/badge/debauchee/barrier.svg)](https://build.snapcraft.io/user/debauchee/barrier)|

Our CI Builds are provided by Microsoft Azure Pipelines, Flathub, and Canonical.

### What is it?

Barrier is software that mimics the functionality of a KVM switch, which historically would allow you to use a single keyboard and mouse to control multiple computers by physically turning a dial on the box to switch the machine you're controlling at any given moment. Barrier does this in software, allowing you to tell it which machine to control by moving your mouse to the edge of the screen, or by using a keypress to switch focus to a different system.

Barrier was forked from Symless's Synergy 1.9 codebase. Synergy was a commercialized reimplementation of the original CosmoSynergy written by Chris Schoeneman.

At the moment, barrier is not compatible with synergy. Barrier needs to be installed on all machines that will share keyboard and mouse.

### What's different?

Whereas Synergy has moved beyond its goals from the 1.x era, Barrier aims to maintain that simplicity.
Barrier will let you use your keyboard and mouse from one computer to control one or more other computers.
Clipboard sharing is supported.
That's it.

### Project goals

Hassle-free reliability. We are users, too. Barrier was created so that we could solve the issues we had with synergy and then share these fixes with other users.

Compatibility. We use more than one operating system and you probably do, too. Windows, OSX, Linux, FreeBSD... Barrier should "just work". We will also have our eye on Wayland when the time comes.

Communication. Everything we do is in the open. Our issue tracker will let you see if others are having the same problem you're having and will allow you to add additional information. You will also be able to see when progress is made and how the issue gets resolved.

### Usage

Install and run barrier on each machine that will be sharing.
On the machine with the keyboard and mouse, make it the server.

Click the "Configure server" button and drag a new screen onto the grid for each client machine.
Ensure the "screen name" matches exactly (case-sensitive) for each configured screen -- the clients' barrier windows will tell you their screen names (just above the server IP).

On the client(s), put in the server machine's IP address (or use Bonjour/auto configuration when prompted) and "start" them.
You should see `Barrier is running` on both server and clients.
You should now be able to move the mouse between all the screens as if they were the same machine.

Note that if the keyboard's Scroll Lock is active then this will prevent the mouse from switching screens.

### Contact & support

Please be aware that the *only* way to draw our attention to a bug is to create a new issue in [the issue tracker](https://github.com/debauchee/barrier/issues). Write a clear, concise, detailed report and you will get a clear, concise, detailed response. Priority is always given to issues that affect a wider range of users.

For short and simple questions or to just say hello find us on the LiberaChat IRC network in the #barrier channel.

### Contributions

At this time we are looking for developers to help fix the issues found in the issue tracker.
Submit pull requests once you've polished up your patch and we'll review and possibly merge it.

Most pull requests will need to include a release note.
See docs/newsfragments/README.md for documentation of how to do that.

## Distro specific packages

While not a comprehensive list, repology provides a decent list of distro
specific packages.

[![Packaging status](https://repology.org/badge/vertical-allrepos/barrier.svg)](https://repology.org/project/barrier/versions)

## FAQ - Frequently Asked Questions

**Q: Does drag and drop work on linux?**

> A: No *(see [#855](https://github.com/debauchee/barrier/issues/544) if you'd like to change that)*


**Q: What OSes are supported?**

> A: The most recent release of Barrier is known to work on:
>  - Windows 7, 8, 8.1, and 10
>  - macOS *(previously known as OS X or Mac OS X)*  
>    - _The current GUI does **not** work on OS versions prior to macOS 10.12 Sierra (but see the related answer below)_
>  - Linux
>  - FreeBSD
>  - OpenBSD


**Q: Are 32-bit versions of Windows supported?**

> A: No


__Q: Is it possible to use Barrier on Mac OS X / OS X versions prior to 10.12?__

> A: Not officially.
>   - For OS X 10.10 Yosemite and later:
>     - [Barrier v2.1.0](https://github.com/debauchee/barrier/releases/tag/v2.1.0) or earlier _may_ work.
>   - For Mac OS X 10.9 Mavericks _(and perhaps earlier)_:
>     1. the command-line portions of the current release _should_ run fine.
>     2. The GUI will _not_ run, as that OS version does not include Apple's *Metal* framework.
>         - _(For a GUI workaround for Mac OS X 10.9, see the [discussion at issue #544](https://github.com/debauchee/barrier/issues/544))_


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
