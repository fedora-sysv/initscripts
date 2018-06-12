# initscripts
This repository contains source code for **legacy** *System V [initscripts](https://en.wikipedia.org/wiki/Init)*,
which are primarily used in *[Linux](https://en.wikipedia.org/wiki/Linux) distributions like e.g.*:
* [Fedora](https://en.wikipedia.org/wiki/Fedora_(operating_system))
* [Red Hat Enterprise Linux](https://en.wikipedia.org/wiki/Red_Hat_Enterprise_Linux)
* [CentOS](https://en.wikipedia.org/wiki/CentOS)
* and some others as well...

Since most of the major Linux distributions have already switched to
*[systemd](https://en.wikipedia.org/wiki/Systemd)*, the *initscripts concept* is
quite outdated nowadays. *As a result, this repository provides primarily only
the support for other initscripts that might still exist out there.*

The above mentioned support includes e.g. the
[`/etc/rc.d/init.d/functions`](https://github.com/fedora-sysv/initscripts/blob/master/etc/rc.d/init.d/functions)
or
[`/usr/sbin/service`](https://github.com/fedora-sysv/initscripts/blob/master/usr/sbin/service)
files.

Another functionality this source code provides includes:
 * [`network-scripts`](https://github.com/fedora-sysv/initscripts/tree/master/network-scripts) - **legacy** scripts for manipulating of network devices
 * [`readonly-root`](https://github.com/fedora-sysv/initscripts/blob/master/usr/lib/systemd/readonly-root) - service for configuring the read-only root support
 * [`netconsole`](https://github.com/fedora-sysv/initscripts/blob/master/etc/rc.d/init.d/netconsole) - service for initializing of network console logging

For the *[RPM](https://en.wikipedia.org/wiki/Rpm_(software))* based distributions
we also provide a [`specfile`](https://github.com/fedora-sysv/initscripts/blob/master/initscripts.spec)
for easier packaging.

## Future of initscripts
As mentioned above, the *initscripts concept* is outdated nowadays, and de-facto
obsolete. Most of the work on this repository is just a maintenance, and we do
**not** plan on extending the support for initscripts in the future in any way.

We intend to convert the rest of the remaining services into *systemd* units,
and remove them eventually if possible.

And we have also started our work on decomissioning of
[`network-scripts`](https://github.com/fedora-sysv/initscripts/tree/master/network-scripts)
as well. This means no new functionality will be added into them. In case you
need some you should ask for it to be implemented
in *[NetworkManager](https://en.wikipedia.org/wiki/NetworkManager)*, if it isn't
already.

## No longer active branches
Follow the steps in our wiki -
[how to access inactive branches](https://github.com/fedora-sysv/initscripts/wiki/How-to-access-inactive-branches) -
in case you need to check out old git branches for any reason.

<!---

BACKUP of old inactive/stale git branches & their hashes:

<branch name>                 <SHA-1 hash>
-------------                 ------------
initscripts-FC1-branch        516fdb9ffde8199d66212241b67956ae21b76bea
initscripts-FC2-branch        f3d2594413456574a0269813bdd351d6b0754924
FC3-branch                    20e2d4679678cf377362de4a62edc8a8e38cf361
FC4-branch                    431a471e479eb6dffc43a3bb785bf2711627fc95
FC5-branch                    28a82ee5ae1f9111d19350a8d342c1fb1e3ed4e6
FC6-branch                    aa8cd70238da8c40db7566e0916aa246bd37abc1
F7-branch                     329556dcf89e0757d49160d9adbe40169571a5d2
F8-branch                     c60ac9fb617a28d6b4a33150b75fbf3d24e05ae4
F9-branch                     301dd44b3fadf1e97644ad462b1ee42043d2f5ef
F10-branch                    210d0fb68b306c50a1e6a4a71cffb57519445d33
F11-branch                    d6f77e0e9c84d0129913dcad6b057fe73e595b89
F12-branch                    5d69a368ab85b007177bc9a5ee38687f0c081708
F13-branch                    d463b24eb479b3aeb5dc51479610fe3decf4ddd5
F14-branch                    55e1e7637b7fc26dceac3d157914d526a79ba18d
F15-branch                    afd5fa70b8ae507736d8619d9b54ee10ac3eaf50
F16-branch                    1baf69352e5f1d3a73827974a937b93eaf3ba9a9
F17-branch                    ccea0dc2f03041056e8b5a07ea9c7baff1c741c2
F18-branch                    7ae0decf762beed8ff17c03fd203e78e3108b46b
initscripts-3_0E-branch       a1d18e8bcb70eb2df53d690cb64138e60cdbb506
initscripts-3_0E-rhgb-branch  6fd5fcccc3aa5fdbdbcab8f7425f87d72ef1a3b4
initscripts-7_0-branch        475ece0115c304cb546dd789a68e1927498f1cf5
initscripts-7_1-branch        e61bc31e8fd37cce1b85aaccd08da8e65a4e377b
initscripts-7_2-branch        27465d15fbdad4142c15b0cd3fbe7de1cb4c9dbd
initscripts-7_3-branch        c222a1c2a46c42c12b5d898091962122375c9c42
initscripts-8_0-branch        826c2b5786a35ba669b9b3f508817c0b64ddc908
initscripts-9-branch          4630ba6b433cb9bbba55d6c942f104535f9c0e0b
origin                        6b5d4bfa26c45eb4351aad41c66d2ab0907cb304
redhat                        578f0bb804e9d26881b3aa2349ff418235d0931b
systemd-branch                cc5b400dd6bad85f5d7b8e4a889134d3668e20a4
SEREL                         8a26159ec668893c845d5dcbec48f509b2dacc6c
unstable                      b5da33084c723b7a182d6724d9a8855a16c0b55d
upstart-0.6.0-branch          5df584569b80bb8977f181e16b0de47fb4df08f1
--->

## Bugs reporting
If you find a bug, we would like to hear about it -- although we can't guarantee
we will be able to fix it... The best way to report bugs differs for each
distribution:
 * `RHEL | CentOS` - create a bug report in [bugzilla](https://bugzilla.redhat.com/enter_bug.cgi) for corresponding RHEL version
 * `Fedora | Other` - create a [new issue](https://github.com/fedora-sysv/initscripts/issues/new) directly here on GitHub

**NOTE:** Bug reports created for *Fedora* in [bugzilla](https://bugzilla.redhat.com/) usually take a lot of time to
resolve. *We advise to use GitHub instead.*
