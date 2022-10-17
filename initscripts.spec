# === GLOBAL MACROS ===========================================================

# According to Fedora Package Guidelines, it is advised that packages that can
# process untrusted input are build with position-idenpendent code (PIC).
#
# Koji should override the compilation flags and add the -fPIC or -fPIE flags by
# default. This is here just in case this wouldn't happen for some reason.
# For more info: https://fedoraproject.org/wiki/Packaging:Guidelines#PIE
%global _hardened_build 1

%global shared_requirements \
Requires:         bash                       \
Requires:         filesystem          >= 3   \
Requires:         coreutils                  \
Requires:         gawk                       \

# =============================================================================

Name:             initscripts
Summary:          Basic support for legacy System V init scripts
Version:          10.17
Release:          %autorelease

License:          GPLv2

URL:              https://github.com/fedora-sysv/initscripts
Source:           https://github.com/fedora-sysv/initscripts/archive/%{version}.tar.gz#/%{name}-%{version}.tar.gz

%shared_requirements

Requires:         findutils
Requires:         grep
Requires:         procps-ng
Requires:         setup
Requires:         systemd
Requires:         util-linux
Requires:         chkconfig
Requires:         initscripts-service
Requires:         initscripts-rename-device

Requires(pre):    shadow-utils
Requires(post):   coreutils

BuildRequires:    filesystem          >= 3
BuildRequires:    gcc
BuildRequires:    git
BuildRequires:    gettext
BuildRequires:    glib2-devel
BuildRequires:    pkgconfig
BuildRequires:    popt-devel
BuildRequires:    setup
BuildRequires:    make

%{?systemd_requires}
BuildRequires:    systemd

Obsoletes:        %{name}            < 10.16-1

# === PATCHES =================================================================

# NOTE: 'autosetup' macro (below) uses 'git' for applying the patches:
#       ->> All the patches should be provided in 'git format-patch' format.
#       ->> Auxiliary repository will be created during 'fedpkg prep', you
#           can see all the applied patches there via 'git log'.

# Upstream patches -- official upstream patches released by upstream since the
# ----------------    last rebase that are necessary for any reason:
#Patch000: example000.patch


# Downstream patches -- these should be always included when doing rebase:
# ------------------
#Patch100: example100.patch


# Downstream patches for RHEL -- patches that we keep only in RHEL for various
# ---------------------------    reasons, but are not enabled in Fedora:
%if %{defined rhel} || %{defined centos}
#Patch200: example200.patch
%endif


# Patches to be removed -- deprecated functionality which shall be removed at
# ---------------------    some point in the future:


%description
This package provides basic support for legacy System V init scripts, and some
other legacy tools & utilities.

# === SUBPACKAGES =============================================================

%package -n initscripts-rename-device
Summary:          Udev helper utility that provides network interface naming

%shared_requirements

%description -n initscripts-rename-device
Udev helper utility that provides network interface naming

# ---------------

%package -n initscripts-service
Summary:          Support for service command
BuildArch:        noarch

%shared_requirements

Requires:         systemd

Provides:         /sbin/service

%description -n initscripts-service
This package provides service command.

# ---------------

%package -n network-scripts
Summary:          Legacy scripts for manipulating of network devices
Requires:         %{name}%{?_isa} = %{version}-%{release}

%shared_requirements

Requires:         bc
Requires:         dbus
Requires:         dbus-tools
Requires:         gawk
Requires:         grep
Requires:         hostname
Requires:         iproute
Requires:         ipcalc
Requires:         kmod
Requires:         procps-ng
Requires:         sed
Requires:         systemd

Requires(post):   chkconfig
Requires(preun):  chkconfig

Requires(post):   %{_sbindir}/update-alternatives
Requires(postun): %{_sbindir}/update-alternatives

Obsoletes:        %{name}            < 9.82-2

# This is legacy and deprecated, so nobody should depend on this!
# If ifcfg-style configuration is still desired, NetworkManager can do this.
# Thus, mark this as deprecated to ensure people know to not depend on it.
# Cf. https://docs.fedoraproject.org/en-US/packaging-guidelines/deprecating-packages/
Provides:         deprecated()

%description -n network-scripts
This package contains the legacy scripts for activating & deactivating of most
network interfaces. It also provides a legacy version of 'network' service.

The 'network' service is enabled by default after installation of this package,
and if the network-scripts are installed alongside NetworkManager, then the
ifup/ifdown commands from network-scripts take precedence over the ones provided
by NetworkManager.

If user has both network-scripts & NetworkManager installed, and wishes to
use ifup/ifdown from NetworkManager primarily, then they has to run command:
 $ update-alternatives --config ifup

Please note that running the command above will also disable the 'network'
service.

# ---------------

%package -n netconsole-service
Summary:          Service for initializing of network console logging
Requires:         %{name} = %{version}-%{release}
BuildArch:        noarch

%shared_requirements

Requires:         glibc-common
Requires:         iproute
Requires:         iputils
Requires:         kmod
Requires:         sed
Requires:         util-linux

Obsoletes:        %{name}            < 9.82-2

%description -n netconsole-service
This packages provides a 'netconsole' service for loading of netconsole kernel
module with the configured parameters. The netconsole kernel module itself then
allows logging of kernel messages over the network.

# ---------------

%package -n readonly-root
Summary:          Service for configuring read-only root support
Requires:         %{name} = %{version}-%{release}
BuildArch:        noarch

%shared_requirements

Requires:         cpio
Requires:         findutils
Requires:         hostname
Requires:         iproute
Requires:         ipcalc
Requires:         util-linux

Obsoletes:        %{name}            < 9.82-2

%description -n readonly-root
This package provides script & configuration file for setting up read-only root
support. Additional configuration is required after installation.

Please note that readonly-root package is considered deprecated with limited support.
Please use systemd-volatile-root functionality instead, if possible.

# === BUILD INSTRUCTIONS ======================================================

%prep
%autosetup -S git

# ---------------

%build
%make_build PYTHON=%{__python3}

# ---------------

%install
%make_install

# This installs the NLS language files:
%find_lang %{name}

%ifnarch s390 s390x
  rm -f %{buildroot}%{_sysconfdir}/sysconfig/network-scripts/ifup-ctc
%endif

# Additional ways to access documentation:
install -m 0755 -d %{buildroot}%{_docdir}/network-scripts

ln -s  %{_docdir}/%{name}/sysconfig.txt %{buildroot}%{_docdir}/network-scripts/
ln -sr %{_mandir}/man8/ifup.8           %{buildroot}%{_mandir}/man8/ifdown.8

# We are now using alternatives approach to better co-exist with NetworkManager:
touch %{buildroot}%{_sbindir}/ifup
touch %{buildroot}%{_sbindir}/ifdown

# =============================================================================

%post
%systemd_post import-state.service loadmodules.service

%preun
%systemd_preun import-state.service loadmodules.service

%postun
%systemd_postun import-state.service loadmodules.service

# ---------------

%post -n network-scripts
chkconfig --add network > /dev/null 2>&1 || :

[ -L %{_sbindir}/ifup ]   || rm -f %{_sbindir}/ifup
[ -L %{_sbindir}/ifdown ] || rm -f %{_sbindir}/ifdown

%{_sbindir}/update-alternatives --install %{_sbindir}/ifup   ifup   %{_sysconfdir}/sysconfig/network-scripts/ifup 90 \
                                --slave   %{_sbindir}/ifdown ifdown %{_sysconfdir}/sysconfig/network-scripts/ifdown \
                                --initscript network

%preun -n network-scripts
if [ $1 -eq 0 ]; then
  chkconfig --del network > /dev/null 2>&1 || :
  %{_sbindir}/update-alternatives --remove ifup %{_sysconfdir}/sysconfig/network-scripts/ifup
fi

# ---------------

%post -n netconsole-service
%systemd_post netconsole.service

%preun -n netconsole-service
%systemd_preun netconsole.service

%postun -n netconsole-service
%systemd_postun netconsole.service

# ---------------

%post -n readonly-root
%systemd_post readonly-root.service

%preun -n readonly-root
%systemd_preun readonly-root.service

%postun -n readonly-root
%systemd_postun readonly-root.service

# === PACKAGING INSTRUCTIONS ==================================================

%files -f %{name}.lang
%license COPYING
%doc doc/sysconfig.txt

# NOTE: /etc/profile.d/ is owned by setup package.
#       /etc/sysconfig/ is owned by filesystem package.
%dir %{_sysconfdir}/rc.d
%dir %{_sysconfdir}/rc.d/init.d
%dir %{_sysconfdir}/rc.d/rc[0-6].d
%dir %{_sysconfdir}/sysconfig/console
%dir %{_sysconfdir}/sysconfig/modules
%dir %{_libexecdir}/%{name}
%dir %{_libexecdir}/%{name}/legacy-actions

# ---------------

%{_sysconfdir}/rc.d/init.d/functions

# RC symlinks:
%{_sysconfdir}/rc[0-6].d

%{_sysconfdir}/init.d

# ---------------

%{_bindir}/*
%{_sbindir}/consoletype
%{_sbindir}/genhostid

%{_libexecdir}/import-state
%{_libexecdir}/loadmodules

%{_prefix}/lib/systemd/system/import-state.service
%{_prefix}/lib/systemd/system/loadmodules.service

%{_mandir}/man1/*

# =============================================================================

%files -n initscripts-rename-device

%{_prefix}/lib/udev/rename_device

%{_udevrulesdir}/*

# ---------------

%files -n initscripts-service

%dir %{_libexecdir}/%{name}
%dir %{_libexecdir}/%{name}/legacy-actions

%{_sbindir}/service

%{_mandir}/man8/service.*

# ---------------

%files -n network-scripts
%doc doc/examples/
%dir %{_sysconfdir}/sysconfig/network-scripts

%{_sysconfdir}/rc.d/init.d/network
%{_sysconfdir}/sysconfig/network-scripts/*

%config(noreplace)    %{_sysconfdir}/sysconfig/network-scripts/ifcfg-lo

%ghost                %{_sbindir}/ifup
%ghost                %{_sbindir}/ifdown
%attr(4755,root,root) %{_sbindir}/usernetctl

%{_mandir}/man8/ifup.*
%{_mandir}/man8/ifdown.*
%{_mandir}/man8/usernetctl.*
%{_docdir}/network-scripts/*

# ---------------

%files -n netconsole-service
%config(noreplace) %{_sysconfdir}/sysconfig/netconsole

%{_libexecdir}/netconsole
%{_prefix}/lib/systemd/system/netconsole.service

# ---------------

%files -n readonly-root
%dir %{_sharedstatedir}/stateless
%dir %{_sharedstatedir}/stateless/state
%dir %{_sharedstatedir}/stateless/writable

%config(noreplace) %{_sysconfdir}/rwtab
%config(noreplace) %{_sysconfdir}/statetab
%config(noreplace) %{_sysconfdir}/sysconfig/readonly-root

%{_libexecdir}/readonly-root
%{_prefix}/lib/systemd/system/readonly-root.service

# =============================================================================

%changelog
%autochangelog
