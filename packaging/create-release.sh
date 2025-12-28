#!/bin/bash
# Release tarball creation script for eiwd

set -e

VERSION=${1:-"0.1.0"}
PKGNAME="eiwd-${VERSION}"
TARBALL="${PKGNAME}.tar.gz"

echo "Creating release tarball for eiwd version ${VERSION}"

# Ensure we're in the project root
cd "$(dirname "$0")/.."

# Clean any existing build artifacts
echo "Cleaning build artifacts..."
rm -rf build/
rm -f "${TARBALL}"

# Create temporary directory for staging
TMPDIR=$(mktemp -d)
STAGEDIR="${TMPDIR}/${PKGNAME}"

echo "Staging files in ${STAGEDIR}..."

# Create staging directory
mkdir -p "${STAGEDIR}"

# Copy source files
cp -r src/ "${STAGEDIR}/"
cp -r data/ "${STAGEDIR}/"
cp -r po/ "${STAGEDIR}/"

# Copy build files
cp meson.build "${STAGEDIR}/"
cp meson_options.txt "${STAGEDIR}/"

# Copy documentation
cp README.md INSTALL.md CONTRIBUTING.md TESTING.md "${STAGEDIR}/"

# Copy license (if exists)
[ -f LICENSE ] && cp LICENSE "${STAGEDIR}/"

# Copy .gitignore
cp .gitignore "${STAGEDIR}/"

# Create tarball
echo "Creating tarball ${TARBALL}..."
tar -czf "${TARBALL}" -C "${TMPDIR}" "${PKGNAME}"

# Generate checksums
echo "Generating checksums..."
sha256sum "${TARBALL}" > "${TARBALL}.sha256"
md5sum "${TARBALL}" > "${TARBALL}.md5"

# Cleanup
rm -rf "${TMPDIR}"

# Display results
echo ""
echo "Release tarball created successfully:"
ls -lh "${TARBALL}"
echo ""
echo "SHA256:"
cat "${TARBALL}.sha256"
echo ""
echo "MD5:"
cat "${TARBALL}.md5"
echo ""
echo "To test the tarball:"
echo "  tar -xzf ${TARBALL}"
echo "  cd ${PKGNAME}"
echo "  meson setup build && ninja -C build"
