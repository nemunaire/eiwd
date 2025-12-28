# Copyright 1999-2025 Gentoo Authors
# Distributed under the terms of the GNU General Public License v2

EAPI=8

inherit meson

DESCRIPTION="Enlightenment Wi-Fi module using iwd backend"
HOMEPAGE="https://git.nemunai.re/nemunaire/eiwd"
SRC_URI="https://git.nemunai.re/nemunaire/eiwd/archive/v${PV}.tar.gz -> ${P}.tar.gz"

LICENSE="BSD"
SLOT="0"
KEYWORDS="~amd64 ~x86 ~arm64"
IUSE="nls"

S="${WORKDIR}/${PN}"

RDEPEND="
	>=x11-wm/enlightenment-0.25.0
	>=dev-libs/efl-1.26.0
	>=net-wireless/iwd-1.0
	sys-apps/dbus
"

DEPEND="${RDEPEND}"

BDEPEND="
	>=dev-build/meson-0.56.0
	virtual/pkgconfig
	nls? ( sys-devel/gettext )
"

DOCS=( README.md INSTALL.md CONTRIBUTING.md )

src_configure() {
	local emesonargs=(
		$(meson_use nls)
	)
	meson_src_configure
}

src_install() {
	meson_src_install
	einstalldocs
}

pkg_postinst() {
	elog "To use eiwd, you need to:"
	elog "1. Ensure iwd service is running: rc-service iwd start"
	elog "2. Enable the module in Enlightenment: Settings -> Modules -> IWD"
	elog "3. Add the gadget to your shelf"
	elog ""
	elog "For non-root Wi-Fi management, configure polkit rules."
	elog "See /usr/share/doc/${PF}/INSTALL.md for details."
}
