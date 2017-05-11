Mounting AFP Volumes in a Shell
===============================

It is possible to mount AFP volumes from an ORCA, GNO, or APW shell command line.  To do this, you should get the `Choose` utility produced by Apple.  It is available on certain old Apple developer CDs, and in the Golden Orchard and Golden Grail software collections available for download [here][1].  On the Golden Orchard CD, the `Choose` command is available under `Programming:Apple:APW & ORCA Commands:Gregs.APW.Utils`.  (The actual command is in the `Utilities` directory, and documentation is in the `Documentation` and `Utilities:Help` directories.)

`Choose` takes an argument of the form `zone:server:volume` to specify the server and volume to connect to.  To connect to a server over TCP/IP, the server name is an IP address or host name, and a special zone name must be used.  This is `AFP over TCP`, optionally followed by a parenthesized list of option codes as given in the table below.  Multiple option codes should be separated by commas.

| Code | Option                           |
|:-----|:---------------------------------|
| `LR` | Use Large Reads                  |
| `LW` | Use Large Writes                 |
| `22` | Force AFP Version 2.2            |
| `FS` | Fake Sleep to Keep Alive         |
| `IE` | Ignore Errors Setting File Types |

As an example, the following command could be used to connect to a server on the local network as a guest, using large reads and large writes:

    Choose -guest "AFP over TCP (LR,LW):192.168.1.10:A2FILES"

See the documentation on `Choose` for more information about its arguments, including how to specify a username and password.

Note that AFPBridge is currently not able to send regular 'tickle' messages to keep AFP connections alive when a text-based shell is active.  This may cause the server to terminate the connection if there is no activity for some length of time.  You can avoid this by using the `FS` (Fake Sleep to Keep Alive) option, if it works with your server.

AFP volumes can be unmounted using the `Eject` command, available in the same place as `Choose`.

[1]: http://digisoft.callapple.org
