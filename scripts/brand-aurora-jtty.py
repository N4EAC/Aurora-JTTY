#!/usr/bin/env python3
"""Apply Aurora JTTY 1.0 branding while retaining existing station data paths."""
import pathlib,sys
source=pathlib.Path(sys.argv[1])
def edit(name,old,new):
 p=source/name;s=p.read_text();assert old in s,(name,old[:90]);p.write_text(s.replace(old,new))
# Operational branding. Storage folder references stay compatible with 0.x.
for name in ('main.cpp','CMakeLists.txt','widgets/mainwindow.cpp','widgets/About.cpp','widgets/SplashScreen.cpp','widgets/JttyLayout.hpp','Configuration.cpp','widgets/mainwindow_settings.cpp','widgets/mainwindow_show_messages.cpp','widgets/mainwindow.ui','Configuration.ui'):
 p=source/name;s=p.read_text();s=s.replace('JTTY Workbench','Aurora JTTY');s=s.replace('/Library/Application Support/Aurora JTTY/Logs','/Library/Application Support/JTTY Workbench/Logs');p.write_text(s)
edit('main.cpp','a.setApplicationVersion (version ());','a.setApplicationVersion ("1.0");\n      a.setApplicationDisplayName ("Aurora JTTY");')
edit('CMakeLists.txt','MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}', 'MACOSX_BUNDLE_BUNDLE_VERSION "1.0"')
edit('CMakeLists.txt','MACOSX_BUNDLE_SHORT_VERSION_STRING "v${wsjtx_VERSION}"','MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"')
edit('CMakeLists.txt','MACOSX_BUNDLE_LONG_VERSION_STRING "Version ${wsjtx_VERSION}"','MACOSX_BUNDLE_LONG_VERSION_STRING "Aurora JTTY 1.0"')
# Existing preference, audio capture and station database locations are retained.
edit('MultiSettings.cpp','QApplication::applicationName () + ".ini"', 'QString(QApplication::applicationName()).replace("Aurora JTTY", "JTTY Workbench") + ".ini"')
edit('Configuration.cpp','writeable_data_dir_ {QStandardPaths::writableLocation (QStandardPaths::DataLocation)}', 'writeable_data_dir_ {QStandardPaths::writableLocation (QStandardPaths::DataLocation).replace("Aurora JTTY", "JTTY Workbench")}')
# The title and status use Aurora's release version; upstream provenance remains in About.
edit('widgets/mainwindow.cpp','version (), revision (),','QCoreApplication::applicationVersion (), revision (),')

# Human-facing startup errors refer to this product; source attribution is intact.
p=source/'main.cpp';p.write_text(p.read_text().replace('WSJT-X', 'Aurora JTTY'))

# Consistent point-size control, including existing and future decoded text.
for name in ('widgets/mainwindow.cpp','widgets/mainwindow.ui','widgets/JttyLayout.hpp'):
 p=source/name;p.write_text(p.read_text().replace('Received messages','All Messages'))
edit('widgets/mainwindow.cpp', 'void MainWindow::applyApplicationStyle (QFont const& font, bool dark)\n{', 'void MainWindow::applyApplicationStyle (QFont const& requestedFont, bool dark)\n{\n  QFont font(requestedFont);\n  int const points=qBound(8,m_settings->value("MainWindow/JttyFontSize",11).toInt(),17);\n  font.setPointSize(points);')
edit('widgets/mainwindow.cpp', '  check_button_color ();\n  updateMainWindowControlSizes ();', '  QFont decodedFont=ui->decodedTextBrowser->contentFont();\n  decodedFont.setPointSize(points);\n  ui->decodedTextBrowser->setContentFont(decodedFont);\n  ui->decodedTextBrowser2->setContentFont(decodedFont);\n  check_button_color ();\n  updateMainWindowControlSizes ();')
edit('widgets/mainwindow.cpp', 'font.pointSizeF () * 1.6', 'font.pointSizeF ()')
edit('widgets/plotter.cpp', 'Font.setPointSize(12);', 'Font.setPointSize(qBound(8, this->font().pointSize(), 17));')
