SUMMARY = "Speech features using TFLite Micro audio frontend"
DESCRIPTION = "Pure C/C++ library that provides speech feature extraction \
using the TensorFlow Lite Micro audio frontend. Provides a clean C API for \
processing 16kHz 16-bit audio samples and extracting features."
HOMEPAGE = "https://github.com/stanleyyyy/pymicro-features-c"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=86d3f3a95c324c9479bd8986968f4327"

SRC_URI = "git://github.com/stanleyyyy/pymicro-features-c.git;protocol=https;branch=main"
SRCREV = "${AUTOREV}"

S = "${WORKDIR}/git"

# Build flags - Makefile will add -DFIXED_POINT=16
EXTRA_OEMAKE = " \
	CC='${CC}' \
	CXX='${CXX}' \
	CFLAGS='${CFLAGS}' \
	CXXFLAGS='${CXXFLAGS}' \
	LDFLAGS='${LDFLAGS}' \
"

# Configure step - clean build directory
do_configure() {
	cd ${S}
	oe_runmake -f Makefile.lib clean
}

do_compile() {
	# Build library and all binaries using Makefile
	cd ${S}
	oe_runmake -f Makefile.lib all test
}

do_install() {
	# Install header files
	install -d ${D}${includedir}
	install -m 0644 ${S}/include/micro_features.h ${D}${includedir}/

	# Install static library
	install -d ${D}${libdir}
	install -m 0644 ${S}/libmicro_features.a ${D}${libdir}/

	# Install example binaries
	install -d ${D}${bindir}
	install -m 0755 ${S}/examples/example_c ${D}${bindir}/
	install -m 0755 ${S}/examples/example_cpp ${D}${bindir}/

	# Install test binary (optional, can be removed if not needed)
	install -m 0755 ${S}/tests/test_micro_features ${D}${bindir}/
}

# Package configuration
PACKAGES = "${PN} ${PN}-dev ${PN}-staticdev ${PN}-examples ${PN}-tests ${PN}-dbg"

FILES:${PN} = ""
FILES:${PN}-dev = "${includedir}/micro_features.h"
FILES:${PN}-staticdev = "${libdir}/libmicro_features.a"
FILES:${PN}-examples = "${bindir}/example_c ${bindir}/example_cpp"
FILES:${PN}-tests = "${bindir}/test_micro_features"
FILES:${PN}-dbg = "${bindir}/.debug"

# Dependencies
DEPENDS = ""

RDEPENDS:${PN} = ""
RDEPENDS:${PN}-examples = "${PN}"
