Name:           e_iwd
Version:        0.1.0
Release:        1%{?dist}
Summary:        Enlightenment Wi-Fi module backed by iwd
License:        GPL-2.0-or-later
URL:            https://example.invalid/e_iwd
Source0:        %{name}-%{version}.tar.xz

BuildRequires:  meson
BuildRequires:  gcc
BuildRequires:  pkgconfig(eldbus)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(enlightenment)

Requires:       enlightenment
Requires:       iwd

%description
Enlightenment shelf module that manages Wi-Fi connections by talking to
the iwd (Intel Wireless Daemon) D-Bus API directly. Replaces the
ConnMan-based econnman gadget.

%prep
%autosetup

%build
%meson
%meson_build

%install
%meson_install

%files
%license COPYING
%doc README.md
%{_libdir}/enlightenment/modules/iwd/

%changelog
* Wed Apr 08 2026 Maintainer <maint@example.invalid> - 0.1.0-1
- Initial scaffolding: D-Bus core, gadcon gadget, popup, agent, config persistence.
