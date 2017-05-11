AFPBridge
=========
By Stephen Heumann

AFPBridge is a tool that allows an Apple IIgs to connect to an AFP (Apple Filing Protocol) file server over TCP/IP.  AFPBridge works by using the existing AppleShare FST, but redirecting its network traffic over TCP/IP rather than AppleTalk.

__To download the latest version of AFPBridge, visit the [releases page][1].__

[1]: https://github.com/sheumann/AFPBridge/releases


System Requirements
-------------------
* An Apple IIgs with AppleTalk enabled in the Control Panel (see below)
* System 6.0.1 or later, with AppleTalk-related components installed (see below)
* Marinetti (latest version recommended) and a compatible network interface

Although AFPBridge uses TCP/IP rather than AppleTalk for networking, it relies on the AppleShare FST and related system components, and these are designed to work with AppleTalk.  Therefore, the system must be configured to use AppleTalk, and the AppleTalk-related components in the system software must be installed and enabled.

If you do not already have the necessary components installed, you can use the "Network: AppleShare" option in the system software installer to add them.  Most of the components installed by this option are necessary in order for AFPBridge to function.  EasyMount (which is also installed by this option) is required in order to use the AFP Mounter control panel.  The AppleShare control panel is not required and cannot be used to connect to AFP servers over TCP/IP; it may be disabled or removed if desired.

In order for the AppleShare FST and the components it requires to load and be usable, it is also necessary to configure the system's slots (in the Slots section of the Control Panel) to enable AppleTalk.  In a ROM 01 system, slot 7 must be set to "AppleTalk" and slot 1 or 2 must be set to "Your Card."  In a ROM 3 system, slot 1 or 2 must be set to "AppleTalk."  Note that it is _not_ actually necessary for the system to be connected to an AppleTalk network (although it may be).

If you wish to use AFPBridge in an emulator, it must be capable of running with AppleTalk enabled as discussed above, although it need not be capable of actually communicating over AppleTalk networks.  GSplus, GSport, and perhaps other KEGS-based emulators should work.  Sweet16 did not work in my testing, because the system hangs during boot-up when AppleTalk is enabled.


Server Compatibility
--------------------
Since the AppleShare FST and related system components are used without modification, they constrain the AFP protocol versions and features that are supported, and accordingly what servers can be used.  In particular, these components are designed to use AFP version 2.0, so it is best to use them with a server that supports that version.  There is an option (described below) to negotiate a connection using AFP version 2.2 instead, which may allow connections to some additional servers, but servers supporting only AFP 3.x cannot be used.

The forms of authentication that can be used are also limited for similar reasons.  The AFP Mounter control panel currently relies on EasyMount to perform authentication, and it supports only guest logins or the 'Randnum exchange'
authentication method.  This method uses a password of up to eight characters protected by weak encryption; it may be disabled on some newer servers.

I recommend using Netatalk 2.x, and in particular the version of it provided by [A2SERVER][2].  This is the primary server platform I have used in my testing, and A2SERVER is designed specifically to work with Apple II systems.  Other AFP 2.x servers, such as the one in Mac OS 9, should also work but have not been tested as extensively.

It is possible to connect to the AFP server in Mac OS X version 10.5 and earlier, but it does not support ProDOS-style file type information, so all files will be shown as having unknown type, and it will generally not be possible to open them in most programs.  With appropriate options (see below) it is possible to write files to Mac OS X servers, but their file types will not be preserved.  The server in Mac OS X also may not support compatible user authentication methods; I have tried it only using guest access.  Later versions of OS X/macOS support only AFP 3.x and cannot be used.  

[2]: http://ivanx.com/a2server/


Installation
------------
To install AFPBridge, place the `AFPBridge` file in the `*:System:System.Setup` folder (where `*` indicates your boot disk), and place the `AFPMounter` file in the `*:System:CDevs` folder.  You must also have Marinetti and the AppleTalk-related system components installed as discussed above.  Reboot the system to complete the installation.


Usage
-----
To connect to an AFP server over TCP/IP, open the __AFP Mounter__ control panel and then enter the URL for a server.  You must specify at least a server and a volume name; there is currently no functionality for listing available servers or for listing the volumes that are available on a server.  If no user name is specified, a guest login will be attempted, and if that is not possible you will be prompted for a user name and password.  If a user name is given without a password, you will be prompted for the password.

The general form of URLs supported for AFP connections over TCP/IP is:

    afp://[user[:password]@]server[:port]/volume

The server can be specified using an IP address or host name.  A host name can only be used if the DNS server you are using can resolve it; other ways of naming systems on the local network are not supported.

It is also possible to use the AFP Mounter control panel to connect to AFP servers over AppleTalk (not relying on AFPBridge).  The form of URLs for that is:

    afp:/at/[user[:password]@]server[:zone]/volume

Click the __Connect__ button to connect to the server, or the __Save Alias...__ button to save an alias to it.  An alias allows you to connect by double-clicking it in the Finder, as long as EasyMount is installed.

There are also several advanced options available in the drop-down menu at the top right of the window.  These apply only to AFP connections over TCP/IP.
The current options will be saved in any alias, as well as applying to connections made directly in the AFP Mounter control panel.

* The __Use Large Reads__ option allows the IIgs to read a larger amount at once than would be allowed with AppleTalk, which improves the speed of large reads from files on the server.  (AppleTalk normally limits the amount of data transmitted at once, requiring multiple network requests to read or write a large amount.)  This option is safe to use with current versions of the AppleShare FST, but in theory it could cause memory corruption or other problems when used with other programs that perform low-level accesses to AFP servers.  I am not aware of any programs that actually cause such problems, so this option should generally be safe to use.

* The __Use Large Writes__ option allows the IIgs to write a larger amount at once than would be allowed with AppleTalk.  This improves write performance, but causes problems with some servers.  This can be safely enabled with Netatalk, but it can cause errors when used with Apple's servers in classic Mac OS or Mac OS X.

* The __Force AFP Version 2.2__ option forces the IIgs to request AFP version 2.2 rather than version 2.0 when connecting to the server.  This can allow connections to some servers that do not support AFP version 2.0.  The AppleShare FST is designed to use AFP version 2.0, so this may cause problems due to the version mismatch.  In practice, these two AFP versions are similar enough that this generally seems to work, but I cannot rule out the possibility of unexpected behavior.

* The __Fake Sleep to Keep Alive__ option may help the IIgs stay connected to the server in cases where it is unexpectedly being disconnected.  This can occur because the server expects the client to periodically send a 'tickle' to indicate that it is still connected.  In some cases (such as when running text-based shells or other non-desktop programs) AFPBridge is not able to do this.  This option works by making the IIgs report to the server that it is going to sleep following each AFP request it sends.  This may cause the server to stay connected without expecting 'tickles,' since a sleeping computer cannot send them.  This option slows down the connection a bit and does not work with all servers.  To use it with Netatalk, you must use AFP version 2.2.

* The __Ignore Errors Setting File Types__ option causes the IIgs to ignore errors when trying to set the ProDOS-style types of files on the server.  This allows the IIgs to write files to servers that do not support ProDOS-style file types (such as the one in old versions of Mac OS X) without being stopped by these errors.  Note that file types will not be correctly set when writing to such servers, even when using this option.

It is also possible to mount AFP volumes from the command line in a shell.  For instructions on doing that, see [here][3].

[3]: https://sheumann.github.io/AFPBridge/ShellMount


Building AFPBridge
------------------

The full source code for AFPBridge is available [on GitHub][4].  For instructions on building it, see [here][5].

[4]: https://github.com/sheumann/AFPBridge
[5]: https://sheumann.github.io/AFPBridge/BUILDING


Contact
-------
If you have questions, comments, bug reports, or feature requests, you can contact me at stephen.heumann@gmail.com or submit them to the [issues page][6] on GitHub.

[6]: https://github.com/sheumann/AFPBridge/issues
