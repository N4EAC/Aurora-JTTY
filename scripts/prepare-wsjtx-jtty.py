#!/usr/bin/env python3
"""Recreate the JTTY-only fork from the pinned, unchanged GPL submodule."""
import io
import pathlib
import shutil
import sys
import subprocess
import tarfile
root = pathlib.Path(__file__).resolve().parents[1]
source = root / 'build/wsjtx-jtty-source'
source.mkdir(parents=True, exist_ok=True)
expected_revision = '567ad29ce6abf3d4a44f181cdbc7ceba0d73e5f4'
revision = subprocess.check_output(['git', '-C', str(root / 'upstream/wsjtx'), 'rev-parse', 'HEAD'], text=True).strip()
if revision != expected_revision:
    raise RuntimeError(f'Expected pinned WSJT-X {expected_revision}, found {revision}')
archive = subprocess.check_output(['git', '-C', str(root / 'upstream/wsjtx'), 'archive', 'HEAD'])
with tarfile.open(fileobj=io.BytesIO(archive)) as files:
    files.extractall(source, filter='data')

def edit(name, old, new):
    path = source / name
    text = path.read_text()
    if old not in text:
        raise RuntimeError(f'Upstream changed: missing anchor in {name}: {old[:80]}')
    path.write_text(text.replace(old, new))

edit('main.cpp', 'a.setApplicationName ("WSJT-X");', 'a.setApplicationName ("JTTY Workbench");')
edit('CMakeLists.txt', 'set (PROJECT_BUNDLE_NAME "WSJT-X")', 'set (PROJECT_BUNDLE_NAME "JTTY Workbench")')
edit('CMakeLists.txt', 'MACOSX_BUNDLE_GUI_IDENTIFIER "org.k1jt.wsjtx"', 'MACOSX_BUNDLE_GUI_IDENTIFIER "com.n4eac.jtty.wsjtx"')
edit('CMakeLists.txt', 'set (PROJECT_DESCRIPTION "WSJT-X: Digital Modes for Weak Signal Communications in Amateur Radio")', 'set (PROJECT_DESCRIPTION "JTTY Workbench: JTTY client derived from WSJT-X")')
# The shared DSP library is retained intact; only JTTY is accessible in this app.
path = source / 'widgets/mainwindow.cpp'
text = path.read_text()
text = text.replace('readSettings();            //Restore user\'s setup parameters', 'readSettings();            //Restore user\'s setup parameters\n  m_mode = "JTTY"; // Independent JTTY-only application, including saved profiles.')
text = text.replace('  ui->actionFreqCal->setActionGroup(modeGroup);', '''  ui->actionFreqCal->setActionGroup(modeGroup);
  for (auto *action : modeGroup->actions()) {
    if (action != ui->actionJTTY) { action->setVisible(false); action->setEnabled(false); }
  }
  for (auto *button : {ui->ft8Button, ui->ft4Button, ui->msk144Button,
                       ui->q65Button, ui->jt65Button, ui->houndButton}) {
    button->setVisible(false); button->setEnabled(false);
  }''')
# Guard direct slots as well as menu actions (shortcuts, profiles, UDP commands).
for mode in ['FST4', 'FST4W', 'FT4', 'FT8', 'JT4', 'JT9', 'JT65', 'Q65', 'MSK144', 'WSPR', 'Echo', 'FreqCal']:
    anchor = f'void MainWindow::on_action{mode}_triggered()\n{{'
    assert anchor in text, mode
    text = text.replace(anchor, anchor + '\n  return; // Other modes are unavailable in the JTTY-only fork.')
text = text.replace('void MainWindow::set_mode (QString const& mode)\n{', 'void MainWindow::set_mode (QString const& mode)\n{\n    Q_UNUSED(mode);\n    on_actionJTTY_triggered();\n    return;')
start = text.index('void MainWindow::set_mode_from_command_line(')
text = text[:start] + '''void MainWindow::set_mode_from_command_line(const QString& mode, bool lock_mode)
{
    Q_UNUSED(mode);
    Q_UNUSED(lock_mode);
    on_actionJTTY_triggered();
}
'''
path.write_text(text)
# Keep the user's original icon while preserving upstream attribution/about text.
iconset = source / 'icons/Darwin/wsjtx.iconset'
shutil.copytree(root / 'native/Assets/JTTY.iconset', iconset, dirs_exist_ok=True)
edit('WSJTXLogging.cpp', 'min_severity["SYSLOG"] = trivial::error;', 'min_severity["SYSLOG"] = trivial::info;')
edit('WSJTXLogging.cpp', 'min_severity["RIGCTRL"] = trivial::warning;', 'min_severity["RIGCTRL"] = trivial::debug;')
shutil.copy2(root / 'native/Assets/JTTY.icns', source / 'icons/JTTY.icns')
edit('CMake/Sources.cmake',
     'COMMAND iconutil -c icns --output "${CMAKE_BINARY_DIR}/${WSJTX_ICON_FILE}" "${CMAKE_SOURCE_DIR}/icons/Darwin/${CMAKE_PROJECT_NAME}.iconset"',
     'COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/icons/JTTY.icns" "${CMAKE_BINARY_DIR}/${WSJTX_ICON_FILE}"')
edit('Audio/soundin.cpp', '  QAudioFormat format (device.preferredFormat());',
     '  LOG_INFO ("JTTY audio input requested: " << device.deviceName().toStdString() << " channel=" << int(channel) << " rate=" << 12000 * downSampleFactor);\n  QAudioFormat format (device.preferredFormat());')
edit('Audio/soundin.cpp', 'void SoundInput::handleStateChanged (QAudio::State newState)\n{',
     'void SoundInput::handleStateChanged (QAudio::State newState)\n{\n  LOG_INFO ("JTTY audio input state=" << int(newState) << " error=" << (m_stream ? int(m_stream->error()) : -1));')
edit('widgets/mainwindow_settings.cpp', 'm_mode=m_settings->value("Mode","FT8").toString();', 'm_mode="JTTY";')
edit('Darwin/Info.plist.in', 'This app requires microphone access to receive signals.', 'Receive JTTY signals from the radio USB audio interface selected in Settings.')
edit('WSJTXLogging.cpp',
     'QDir app_data {QStandardPaths::writableLocation (QStandardPaths::AppLocalDataLocation)};',
     '''QDir app_data {QStandardPaths::isTestModeEnabled()
      ? QStandardPaths::writableLocation (QStandardPaths::AppLocalDataLocation)
      : QDir::homePath() + "/Library/Application Support/JTTY Workbench/Logs"};
    app_data.mkpath(".");
    app_data.mkpath("logs");''')
edit('WSJTXLogging.cpp', 'keywords::auto_flush = false', 'keywords::auto_flush = true')
edit('widgets/mainwindow.h', '  void set_mode_from_command_line(const QString& mode, bool lock_mode = false);',
     '  QString jttyOperatingMode() const { return m_mode; }\n  void set_mode_from_command_line(const QString& mode, bool lock_mode = false);')
edit('main.cpp', '                smoke_phase ("event loop reached");',
     '''                smoke_phase ("event loop reached");
                w.set_mode_from_command_line("FT8", true);
                if (w.jttyOperatingMode() != "JTTY") {
                  std::cerr << "JTTY-only mode restriction failed" << std::endl;
                  a.exit(EXIT_FAILURE);
                  return;
                }
                std::cout << "JTTY-only mode restriction passed" << std::endl;''')
# JTTY uses the upstream in-process fast decoder, not the jt9 IPC process.
# Avoid its large SysV segment, which requires system tuning on macOS.
path = source / 'main.cpp'
text = path.read_text()
start = text.index('          // Create and initialize shared memory segment')
end = text.index('          unsigned downSampleFactor;', start)
text = text[:start] + '          // JTTY has no jt9 subprocess or shared decoder segment.\n\n' + text[end:]
path.write_text(text)
edit('widgets/mainwindow.cpp', 'void MainWindow::startDecoderProcess ()\n{',
     'void MainWindow::startDecoderProcess ()\n{\n  return; // JTTY decodes in process; the multimode jt9 backend is unused.')

# Record native device identity and stop if the selected device disappears.
shutil.copy2(root / 'native/qt-audio/JttyCaptureRouteWatch.hpp', source / 'Audio/JttyCaptureRouteWatch.hpp')
edit('CMakeLists.txt', 'target_link_libraries (wsjt_qtmm Qt5::Multimedia)', 'target_link_libraries (wsjt_qtmm Qt5::Multimedia)\nif(APPLE)\n  target_link_libraries(wsjt_qtmm "-framework CoreAudio" "-framework CoreFoundation")\nendif()')
edit('Audio/soundin.cpp', '#include "Logger.hpp"', '#include "Logger.hpp"\n#include "JttyCaptureRouteWatch.hpp"')
edit('Audio/soundin.h', 'class QAudioInput;', 'class QAudioInput;\n#ifdef Q_OS_MAC\nclass JttyCaptureRouteWatch;\n#endif')
edit('Audio/soundin.h', '  QScopedPointer<QAudioInput> m_stream;', '  QScopedPointer<QAudioInput> m_stream;\n#ifdef Q_OS_MAC\n  QScopedPointer<JttyCaptureRouteWatch> m_routeWatch;\n#endif')
edit('Audio/soundin.h', '  SoundInput (QObject * parent = nullptr)\n    : AudioInputSource {parent}\n    , cummulative_lost_usec_ {std::numeric_limits<qint64>::min ()}\n  {\n  }', '  SoundInput (QObject * parent = nullptr);')
edit('Audio/soundin.cpp', 'bool SoundInput::checkStream ()', 'SoundInput::SoundInput(QObject *parent):AudioInputSource{parent},cummulative_lost_usec_{std::numeric_limits<qint64>::min()}{ }\n\nbool SoundInput::checkStream ()')
edit('Audio/soundin.cpp', '  m_sink = sink;', '''  QAudioDeviceInfo selected;
  for(auto const& candidate:QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
    if(candidate.deviceName()==device.deviceName()) {selected=candidate;break;}
  if(selected.isNull()) { Q_EMIT error(tr("Selected input device is unavailable. Refresh Radio / Audio settings.")); return; }
#ifdef Q_OS_MAC
  m_routeWatch.reset(new JttyCaptureRouteWatch(selected.deviceName(),[this](QString message){
    if(m_stream) m_stream->stop();
    clearStreamDescriptor();
    Q_EMIT error(message);
  },[this]{return QString("qt_state=%1 qt_error=%2 processed_us=%3").arg(m_stream?int(m_stream->state()):-1).arg(m_stream?int(m_stream->error()):-1).arg(m_stream?m_stream->processedUSecs():0);}));
  if(!m_routeWatch->valid()){Q_EMIT error(tr("Cannot uniquely identify the selected CoreAudio input device."));return;}
#endif
  m_sink = sink;''')
edit('Audio/soundin.cpp', 'QAudioFormat format (device.preferredFormat())', 'QAudioFormat format (selected.preferredFormat())')
edit('Audio/soundin.cpp', 'device.isFormatSupported (format)', 'selected.isFormatSupported (format)')
edit('Audio/soundin.cpp', 'new QAudioInput {device, format}', 'new QAudioInput {selected, format}')
edit('Audio/soundin.cpp', '          publishStreamDescriptor ();', '          publishStreamDescriptor ();\n#ifdef Q_OS_MAC\n          if(m_routeWatch) m_routeWatch->begin();\n#endif')
edit('Audio/soundin.cpp', 'void SoundInput::resume ()\n{', 'void SoundInput::resume ()\n{\n  LOG_INFO("JTTY capture resume requested");\n#ifdef Q_OS_MAC\n  if(m_routeWatch){m_routeWatch->sample();if(!m_routeWatch->valid())return;}\n#endif')
edit('Audio/soundin.cpp', 'void SoundInput::suspend ()\n{', 'void SoundInput::suspend ()\n{\n  LOG_INFO("JTTY capture suspend requested");')
edit('Audio/soundin.cpp', 'void SoundInput::stop()\n{', 'void SoundInput::stop()\n{\n  LOG_INFO("JTTY capture stop requested");\n#ifdef Q_OS_MAC\n  m_routeWatch.reset();\n#endif')

subprocess.run([sys.executable, str(root / 'scripts/deepen-jtty-debug.py'), str(source)], check=True)

subprocess.run([sys.executable, str(root / 'scripts/customize-jtty-ui.py'), str(source)], check=True)
print(source)
