import qbs

Application {
	Depends { name: "cpp" }

	files: [
        "LineInputHandler.cpp",
        "LineInputHandler.hpp",
        "mailbox.c",
        "mailbox.h",
        "main.cpp",
        "mem.c",
        "mem.h",
        "reg.h",
    ]

	cpp.includePaths: [
		cpp.sysroot + "/usr/include",
		cpp.sysroot + "/usr/include/c++/5.2.0",
		cpp.sysroot + "/usr/include/c++/5.2.0/armv7l-unknown-linux-gnueabihf"
	]

	cpp.cFlags: [
		"-pthread", "-target", "armv7l-unknown-linux-gnueabihf"
	]
	cpp.cxxFlags: cpp.cFlags.concat(["-std=c++14"])

	cpp.linkerFlags: [
		"-pthread", "-fuse-ld=gold", "-target", "armv7l-unknown-linux-gnueabihf"
	]

	cpp.dynamicLibraries: [
		"boost_system"
	]

	Group {
		fileTagsFilter: product.type
		qbs.install: true
		qbs.installDir: "/usr/local/bin"
	}
}
