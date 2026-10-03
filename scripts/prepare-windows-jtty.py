#!/usr/bin/env python3
"""Windows-specific branding and Hamlib-only CAT build overlay."""
import pathlib
import sys
source = pathlib.Path(sys.argv[1])
def edit(name, old, new):
    p = source / name
    text = p.read_text()
    assert old in text, (name, old[:80])
    p.write_text(text.replace(old, new))
# Mac storage compatibility stays unchanged; Windows uses its native application-data directory.
edit('WSJTXLogging.cpp', '    QDir app_data {QStandardPaths::isTestModeEnabled()', '#ifdef Q_OS_MAC\n    QDir app_data {QStandardPaths::isTestModeEnabled()')
edit('WSJTXLogging.cpp', '    app_data.mkpath(".");', '#else\n    QDir app_data {QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/Logs"};\n#endif\n    app_data.mkpath(".");')
# No proprietary COM dependency is needed for Hamlib CAT/PTT.
edit('CMake/Sources.cmake', '    Transceiver/OmniRigTransceiver.cpp', '    # Aurora uses Hamlib rather than OmniRig')
edit('CMake/Sources.cmake', 'if (WIN32)\n  # generate the OmniRig COM interface source', 'if (FALSE)\n  # OmniRig is disabled in Aurora JTTY')
edit('CMakeLists.txt', 'if (WIN32)\n  include (QtAxMacros)', 'if (WIN32 AND AXSERVERSRCS)\n  include (QtAxMacros)')
edit('Transceiver/TransceiverFactory.cpp', '#if defined (WIN32)', '#if defined (WIN32) && defined (JTTY_ENABLE_OMNIRIG)')
# The version resource generator reads the Windows ICO from this path.
import shutil
shutil.copy2(pathlib.Path(__file__).resolve().parent.parent / 'native/Assets/Aurora-JTTY.ico', source / 'icons/windows-icons/wsjtx.ico')

# Windows Explorer shows Aurora's release version, while source provenance remains in About.
edit('CMake/Modules/generate_version_info.cmake', '# Force resource files into the top-level build directory', 'if (WIN32 AND PRODUCT_NAME STREQUAL "wsjtx")\n  set(PRODUCT_VERSION_MAJOR 1)\n  set(PRODUCT_VERSION_MINOR 0)\n  set(PRODUCT_VERSION_PATCH 0)\n  set(PRODUCT_VERSION_TWEAK 0)\n  set(PRODUCT_VERSION_REVISION "")\n  set(PRODUCT_FILE_DESCRIPTION "Aurora JTTY")\n  set(PRODUCT_ORIGINAL_FILENAME "Aurora-JTTY.exe")\nendif ()\n# Force resource files into the top-level build directory')

# Reparented Aurora widgets must not inherit the original Windows FT8 sizing policy.
edit('widgets/mainwindow.cpp', 'void MainWindow::updateMainWindowControlSizes()\n{', 'void MainWindow::updateMainWindowControlSizes()\n{\n#ifdef Q_OS_WIN\n  if(findChild<QWidget *>("jttyWorkbenchRoot")) {\n    for(auto button:{ui->lookupButton,ui->addButton,ui->ignoreButton}) {\n      button->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);\n      button->setMinimumWidth(70);\n      button->setMaximumWidth(110);\n    }\n    return;\n  }\n#endif')
