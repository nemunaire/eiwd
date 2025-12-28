# eiwd Packaging

Distribution-specific packaging files for eiwd.

## Directory Structure

```
packaging/
├── arch/
│   └── PKGBUILD              # Arch Linux package
├── gentoo/
│   └── eiwd-0.1.0.ebuild     # Gentoo ebuild
├── create-release.sh         # Release tarball generator
└── README.md                 # This file
```

## Creating a Release Tarball

```bash
# From project root
./packaging/create-release.sh 0.1.0

# This creates:
# - eiwd-0.1.0.tar.gz
# - eiwd-0.1.0.tar.gz.sha256
# - eiwd-0.1.0.tar.gz.md5
```

The tarball includes:
- Source code (src/, data/, po/)
- Build system (meson.build, meson_options.txt)
- Documentation (README.md, INSTALL.md, etc.)
- License file (if present)

## Arch Linux Package

### Building Locally

```bash
cd packaging/arch/

# Download/create source tarball
# Update sha256sums in PKGBUILD

# Build package
makepkg -si

# Or just build without installing
makepkg
```

### Publishing to AUR

1. Create AUR account: https://aur.archlinux.org/register
2. Set up SSH key: https://wiki.archlinux.org/title/AUR_submission_guidelines
3. Clone AUR repository:
   ```bash
   git clone ssh://aur@aur.archlinux.org/eiwd.git
   cd eiwd
   ```
4. Copy PKGBUILD and update:
   - Set correct `source` URL
   - Update `sha256sums` with actual checksum
   - Add .SRCINFO:
     ```bash
     makepkg --printsrcinfo > .SRCINFO
     ```
5. Commit and push:
   ```bash
   git add PKGBUILD .SRCINFO
   git commit -m "Initial import of eiwd 0.1.0"
   git push
   ```

### Testing Installation

```bash
# Install from local PKGBUILD
cd packaging/arch/
makepkg -si

# Verify installation
pacman -Ql eiwd
ls -R /usr/lib/enlightenment/modules/iwd/

# Test module
enlightenment_remote -module-load iwd
```

## Gentoo Package

### Adding to Local Overlay

```bash
# Create overlay if needed
mkdir -p /usr/local/portage/x11-plugins/eiwd

# Copy ebuild
cp packaging/gentoo/eiwd-0.1.0.ebuild \
   /usr/local/portage/x11-plugins/eiwd/

# Generate manifest
cd /usr/local/portage/x11-plugins/eiwd
ebuild eiwd-0.1.0.ebuild manifest

# Install
emerge -av eiwd
```

### Testing Installation

```bash
# Build and install
emerge eiwd

# Verify files
equery files eiwd

# Test module
enlightenment_remote -module-load iwd
```

### Submitting to Gentoo Repository

1. Create bug report: https://bugs.gentoo.org/
2. Attach ebuild and provide:
   - Package description
   - Upstream URL
   - License verification
   - Testing information (architecture, E version)
3. Monitor for maintainer feedback
4. Address any requested changes

## Debian/Ubuntu Package

To create a .deb package:

```bash
# Install packaging tools
sudo apt install devscripts build-essential debhelper

# Create debian/ directory structure
mkdir -p debian/source

# Create required files:
# - debian/control (package metadata)
# - debian/rules (build instructions)
# - debian/changelog (version history)
# - debian/copyright (license info)
# - debian/source/format (package format)

# Build package
debuild -us -uc

# Install
sudo dpkg -i ../eiwd_0.1.0-1_amd64.deb
```

Example `debian/control`:
```
Source: eiwd
Section: x11
Priority: optional
Maintainer: Your Name <your.email@example.com>
Build-Depends: debhelper (>= 13), meson, ninja-build, libefl-all-dev, enlightenment-dev
Standards-Version: 4.6.0
Homepage: https://github.com/yourusername/eiwd

Package: eiwd
Architecture: any
Depends: ${shlibs:Depends}, ${misc:Depends}, enlightenment (>= 0.25), iwd (>= 1.0)
Recommends: polkit
Description: Enlightenment Wi-Fi module using iwd backend
 eiwd provides native Wi-Fi management for the Enlightenment desktop
 environment using Intel Wireless Daemon (iwd) as the backend.
 .
 Features fast scanning, secure authentication, and seamless
 integration with Enlightenment's module system.
```

## Fedora/RPM Package

```bash
# Install packaging tools
sudo dnf install rpmdevtools rpmbuild

# Set up RPM build tree
rpmdev-setuptree

# Create spec file in ~/rpmbuild/SPECS/eiwd.spec
# Copy tarball to ~/rpmbuild/SOURCES/

# Build RPM
rpmbuild -ba ~/rpmbuild/SPECS/eiwd.spec

# Install
sudo rpm -i ~/rpmbuild/RPMS/x86_64/eiwd-0.1.0-1.fc39.x86_64.rpm
```

Example spec file excerpt:
```spec
Name:           eiwd
Version:        0.1.0
Release:        1%{?dist}
Summary:        Enlightenment Wi-Fi module using iwd backend

License:        BSD
URL:            https://github.com/yourusername/eiwd
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  meson ninja-build gcc
BuildRequires:  enlightenment-devel efl-devel
Requires:       enlightenment >= 0.25
Requires:       efl >= 1.26
Requires:       iwd >= 1.0

%description
eiwd provides native Wi-Fi management for Enlightenment using iwd.

%prep
%autosetup

%build
%meson
%meson_build

%install
%meson_install

%files
%license LICENSE
%doc README.md INSTALL.md
%{_libdir}/enlightenment/modules/iwd/
```

## Generic Installation from Tarball

For distributions without packages:

```bash
# Extract tarball
tar -xzf eiwd-0.1.0.tar.gz
cd eiwd-0.1.0

# Build and install
meson setup build --prefix=/usr/local
ninja -C build
sudo ninja -C build install

# Module will be installed to:
# /usr/local/lib64/enlightenment/modules/iwd/
```

## Packaging Checklist

Before releasing a package:

- [ ] Version number updated in:
  - [ ] `meson.build` (project version)
  - [ ] PKGBUILD (pkgver)
  - [ ] ebuild (filename and PV)
  - [ ] debian/changelog
  - [ ] spec file

- [ ] Source tarball created and tested:
  - [ ] Extracts cleanly
  - [ ] Builds successfully
  - [ ] All files included
  - [ ] Checksums generated

- [ ] Documentation up to date:
  - [ ] README.md reflects current features
  - [ ] INSTALL.md has correct paths
  - [ ] CONTRIBUTING.md guidelines current

- [ ] Package metadata correct:
  - [ ] Dependencies accurate
  - [ ] License specified
  - [ ] Homepage/URL set
  - [ ] Description clear

- [ ] Installation tested:
  - [ ] Module loads in Enlightenment
  - [ ] Files installed to correct paths
  - [ ] No missing dependencies
  - [ ] Uninstall works cleanly

- [ ] Distribution-specific:
  - [ ] Arch: .SRCINFO generated
  - [ ] Gentoo: Manifest created
  - [ ] Debian: Lintian clean
  - [ ] Fedora: rpmlint passes

## Version Numbering

Follow semantic versioning (semver):

- **0.1.0** - Initial release
- **0.1.1** - Bug fix release
- **0.2.0** - New features (backward compatible)
- **1.0.0** - First stable release
- **1.1.0** - New features post-1.0
- **2.0.0** - Breaking changes

## Distribution Maintainer Notes

### System Integration

Packages should:
- Install to standard library paths (`/usr/lib64` or `/usr/lib`)
- Include documentation in `/usr/share/doc/eiwd/`
- Not conflict with other Wi-Fi managers
- Recommend but not require polkit

### Dependencies

**Build-time**:
- meson >= 0.56
- ninja
- gcc/clang
- pkg-config
- edje_cc (part of EFL)

**Runtime**:
- enlightenment >= 0.25
- efl >= 1.26 (elementary, eldbus, ecore, evas, edje, eina)
- iwd >= 1.0
- dbus

**Optional**:
- polkit (for non-root Wi-Fi management)
- gettext (for translations)

### Post-Install

Inform users to:
1. Enable iwd service
2. Configure polkit rules (provide example)
3. Load module in Enlightenment
4. Add gadget to shelf

### Known Issues

- Conflicts with wpa_supplicant (both should not run simultaneously)
- Requires D-Bus system bus access
- May need additional polkit configuration on some distributions

## Support

For packaging questions:
- Open an issue on GitHub
- Check distribution-specific guidelines
- Refer to INSTALL.md for detailed setup

## Resources

- [Arch Linux Packaging Standards](https://wiki.archlinux.org/title/Arch_package_guidelines)
- [Gentoo ebuild Writing Guide](https://devmanual.gentoo.org/ebuild-writing/)
- [Debian Packaging Tutorial](https://www.debian.org/doc/manuals/maint-guide/)
- [Fedora RPM Guide](https://docs.fedoraproject.org/en-US/packaging-guidelines/)
- [iwd Documentation](https://iwd.wiki.kernel.org/)
