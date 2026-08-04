import qbs 1.0

QtcPlugin {
    name: "DsComplete"

    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "Utils" }
    Depends { name: "Qt"; submodules: ["network", "widgets"] }

    files: [
        "dscompleteclient.cpp",
        "dscompleteclient.h",
        "dscompleteconstants.h",
        "dscompleteplugin.cpp",
        "dscompleteprotocol.cpp",
        "dscompleteprotocol.h",
        "dscompletesettings.cpp",
        "dscompletesettings.h",
        "dscompletetr.h",
    ]

    QtcTestFiles {
        files: [
            "tst_dscompleteprotocol.cpp",
        ]
    }
}