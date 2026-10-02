#!/usr/bin/env python3
"""Apply presentation changes to the prepared JTTY-only source tree."""
import pathlib
import re
import shutil
import sys
import xml.etree.ElementTree as ET
root = pathlib.Path(__file__).resolve().parents[1]
source = pathlib.Path(sys.argv[1])
def edit(name, old, new):
    path = source / name
    text = path.read_text()
    assert old in text, (name, old[:100])
    path.write_text(text.replace(old, new))

# Save the last partial JTTY receive buffer on every monitoring stop path.
# JTTY decoding is synchronous, so its decoded flag is final at this point.
edit('widgets/mainwindow.cpp', 'void MainWindow::monitor (bool state)\n{', '''void MainWindow::monitor (bool state)
{
  if (!state && m_monitoring && m_mode=="JTTY" && !m_diskData
      && (m_saveAll || m_saveDecoded)) jtty_save_wav();''')
edit('widgets/mainwindow_jtty.cpp', '''  // Reject callers that arrive before a real JTTY capture exists; m_k0 is''', '''  if (!m_saveAll && (!m_saveDecoded || !m_bDecoded)) return;
  // Reject callers that arrive before a real JTTY capture exists; m_k0 is''')
edit('widgets/mainwindow_jtty.cpp', '''  // "Save decoded" keeps the file only if something was decoded; give the
  // decoder a further 3 seconds to finish before killWaveFile() decides.
  if (m_saveDecoded) killFileTimer.start (3000);''', '''  // Keep this capture once admitted; a later receive interval must not
  // delete it after resetting m_bDecoded.
  killFileTimer.stop();
  LOG_INFO("JTTY WAV queued path=" << (m_fnameWE+".wav").toStdString()
           << " samples=" << samples << " decoded=" << m_bDecoded
           << " save_all=" << m_saveAll);''')

# JTTY rows contain an optional UTC column followed by the audio frequency.
# Selecting a frequency must never treat that number as a remote callsign.
edit('widgets/mainwindow.cpp', '''    m_deCall = word;
    ui->dxCallEntry->setText(m_deCall);
    return;''', r'''    static QRegularExpression const header {
      QStringLiteral("^\\s*(?:[0-2][0-9][0-5][0-9][0-5][0-9]\\s+)?(?:[-+]?[0-9]{1,3}\\s+)?([0-9]{1,4})(?:\\s|$)")};
    auto const match = header.match(line);
    if (match.hasMatch()) {
      int const frequency = match.captured(1).toInt();
      if (frequency >= ui->RxFreqSpinBox_2->minimum()
          && frequency <= ui->RxFreqSpinBox_2->maximum()) {
        ui->RxFreqSpinBox_2->setValue(frequency);
      }
    }
    auto const call = word.trimmed().toUpper();
    static QRegularExpression const callCharacters {
      QStringLiteral("^[A-Z0-9]+(?:/[A-Z0-9]+)*$")};
    if (callCharacters.match(call).hasMatch() && Radio::is_callsign(call)) {
      m_deCall = call;
      ui->dxCallEntry->setText(m_deCall);
    }
    return;''')

# Leave attribution documents and the copyright dialog unchanged. Replace only
# operational text literals, never includes, identifiers or source URLs.
for name in ['main.cpp', 'Configuration.cpp', 'widgets/mainwindow.cpp', 'widgets/mainwindow_settings.cpp', 'widgets/mainwindow_show_messages.cpp']:
    path = source / name
    def brand(match):
        value = match.group()
        if 'http' not in value:
            value = value.replace('WSJT-X™', 'JTTY Workbench').replace('WSJT-X', 'JTTY Workbench')
        return value
    path.write_text(re.sub(r'"(?:[^"\\]|\\.)*"', brand, path.read_text()))

menus = {
 'menuFile': {'actionOpen','actionOpen_next_in_directory','actionDecode_remaining_files_in_directory','actionDelete_all_wav_files_in_SaveDir','actionErase_ALL_TXT','actionErase_wsjtx_log_adi','actionErase_Tx_Log','actionErase_Ignore_List','reset_cabrillo_log_action','actionExport_Cabrillo_log','actionOpen_log_directory','actionSettings','actionExit'},
 'menuView': {'actionWide_Waterfall','contest_log_action','actionColors','actionBand_Buttons'},
 'menuDecode': {'actionDisable_clicks_on_waterfall','actionFull_Duplex_Mode'},
 'menuMode': {'actionJTTY'},
 'menuHelp': {'actionOnline_User_Guide','actionCopyright_Notice','actionTrademark_Policy','actionAbout'},
 'menuTools': set(),
}
tree = ET.parse(source / 'widgets/mainwindow.ui')
removed = set()
for widget in tree.iter('widget'):
    name = widget.get('name')
    if name in menus:
        for action in list(widget.findall('addaction')):
            if action.get('name') not in menus[name]:
                removed.add(action.get('name')); widget.remove(action)
for action in tree.iter('action'):
    if action.get('name') in removed:
        for prop in list(action.findall('property')):
            if prop.get('name') == 'shortcut': action.remove(prop)
    for text in action.iter('string'):
        text.text = (text.text or '').replace('WSJT-X™', 'JTTY Workbench').replace('WSJT-X', 'JTTY Workbench')
for text in tree.iter('string'):
    if text.text == 'WSJT-X™   by K1JT et al.': text.text = 'JTTY Workbench'
    if text.text == 'Online User Guide': text.text = 'JTTY User Guide'
ET.indent(tree, space=' ')
tree.write(source / 'widgets/mainwindow.ui', encoding='UTF-8', xml_declaration=True)

# These operational controls are inapplicable to a continuous JTTY chat.
# Apply after the upstream mode layout so saved settings cannot re-show them.
hide_widgets = ['txFirstCheckBox','rptSpinBox','cbAutoSeq','respondComboBox','cbHoldTxFreq','pbR2T','pbT2R','DecodeButton','autoButton','cbSWL','cbShMsgs','cbFast9','cbCQonly','pbBestSP','band_hopping_group_box','cbBypass']
cleanup = '\n'.join(f'  ui->{name}->hide();' for name in hide_widgets)
cleanup += '''
  m_reportLabel->hide();
  setTrPeriodVisible(false);
  ui->menuTools->menuAction()->setVisible(false);
  ui->menuFilters->menuAction()->setVisible(false);
  ui->actionUse_Dark_Style->setChecked(false);
  ui->actionUse_Dark_Style->setVisible(false);
  setDecodeTitles(tr("Received messages"), tr("Conversation"));
  ui->Tx_Message->setPlaceholderText(tr("Type a JTTY message — Enter to send"));
  setWindowTitle("JTTY Workbench");
'''
edit('widgets/mainwindow.cpp', '  monitor(true);\n}\n\n\nvoid MainWindow::on_actionMSK144_triggered()', '  monitor(true);\n'+cleanup+'}\n\n\nvoid MainWindow::on_actionMSK144_triggered()')
# Make saved dark-mode preferences consistent with the neutral gray theme.
edit('widgets/mainwindow_settings.cpp', 'ui->actionUse_Dark_Style->setChecked(', 'ui->actionUse_Dark_Style->setChecked(false && ')

# Keep settings indices stable: hide tabs rather than removing their objects.
config_hide = ['enable_VHF_features_check_box','auto_astro_check_box','repeat_Tx_check_box','decode_at_52s_check_box','single_decode_check_box','alternate_bindings_check_box','quick_call_check_box','disable_TX_on_73_check_box','force_call_1st_check_box','enable_Wait_features_check_box','azel_path_group_box','groupBox_5','groupBox_10','groupBox_6','gbSpecialOpActivity']
config_cleanup = '\n'.join(f'  ui_->{name}->hide();' for name in config_hide)
config_cleanup += '\n  ui_->configuration_tabs->setTabVisible(ui_->configuration_tabs->indexOf(ui_->tx_macros_tab), false);'
config_cleanup += '\n  ui_->configuration_tabs->setTabVisible(ui_->configuration_tabs->indexOf(ui_->filters_tab), false);'
edit('Configuration.cpp', '  return QDialog::exec();', config_cleanup+'\n  return QDialog::exec();')

# Gray theme follows fonts across all settings changes and auxiliary windows.
css = (root/'native/qt-ui/gray.qss').read_text()
dark_css = (root/'native/qt-ui/dark.qss').read_text()
edit('qt_helpers.cpp', '  return style_sheet + "* {" + font_as_stylesheet (font) + \'}\';',
     '  return "* {" + font_as_stylesheet (font) + \'}\' + (dark_style ? QString::fromUtf8(R"JTTYDARK('+dark_css+')JTTYDARK") : QString::fromUtf8(R"JTTYGRAY('+css+')JTTYGRAY"));')
edit('widgets/mainwindow.cpp', 'void MainWindow::applyApplicationStyle (QFont const& font, bool dark)\n{', 'void MainWindow::applyApplicationStyle (QFont const& font, bool dark)\n{\n  dark = m_settings->value("MainWindow/JttyDarkTheme",false).toBool();')

edit('widgets/About.cpp', 'WSJT-X™ v', 'JTTY Workbench v')
edit('widgets/About.cpp', 'WSJT-X™ implements a number of digital modes designed for <br />"\n    "weak-signal Amateur Radio communication.', 'JTTY Workbench provides keyboard-to-keyboard JTTY communication.<br />"\n    "Based on the open-source WSJT-X project.')
edit('widgets/About.cpp', 'WSJT-X™ is licensed', 'JTTY Workbench is licensed')
# The original copyright list and GPL/source-project attribution remain intact.
edit('widgets/SplashScreen.cpp', ': QSplashScreen {QPixmap {":/splash.png"}, Qt::WindowStaysOnTopHint}', ': QSplashScreen {[] { QPixmap image(640, 340); image.fill(QColor("#d0d2d5")); return image; }(), Qt::WindowStaysOnTopHint}')
path=source/'widgets/SplashScreen.cpp';text=path.read_text().replace('WSJT-X™','JTTY Workbench').replace('WSJT-X startup','JTTY Workbench startup');text=text.replace('Send issue reports to https://wsjtx.groups.io, and be sure to save .wav<br />"\n    "files where appropriate.', 'JTTY keyboard-to-keyboard radio communication.<br />"\n    "Choose your radio and USB audio interface in Settings.');text=text.replace('Open the Help menu and select Release Notes for more details.', 'Open Help → JTTY User Guide for operating instructions.');path.write_text(text)
# Add practical JTTY help in place of the generic multimode manual.
p=source/'widgets/mainwindow.cpp';text=p.read_text();anchor='void MainWindow::on_actionOnline_User_Guide_triggered()';start=text.index(anchor);body=text.index('{',start);end=text.index('\n}',body)
text=text[:body]+'''{
  QMessageBox::information(this, tr("JTTY User Guide"),
    tr("Configure your callsign, radio CAT/PTT, and USB input/output in Settings.\\n\\n"
       "Choose a JTTY dial frequency and press Monitor. Received messages appear in the left panel; "
       "the right panel follows the selected audio frequency. Click the waterfall to select a signal.\\n\\n"
       "Type up to 80 characters in the message field. Press Enter or Send message to transmit. "
       "F1–F8 send macros; edit their text in the fields beneath the buttons. Halt Tx stops transmission."));
''' + text[end:];p.write_text(text)
# App icon in About/splash is the original JTTY icon, too.
for p in (source/'icons').rglob('icon_128x128.png'):
    shutil.copy2(root/'native/Assets/JTTY.iconset/icon_128x128.png', p)

# Rebrand settings and waterfall tooltips as well as the main menus.
for name in ['Configuration.ui', 'widgets/widegraph.ui']:
    path = source / name
    tree = ET.parse(path)
    for text in tree.iter('string'):
        if text.text and 'http' not in text.text:
            text.text = text.text.replace('WSJT-X™','JTTY Workbench').replace('WSJT-X','JTTY Workbench')
    ET.indent(tree, space=' ')
    tree.write(path, encoding='UTF-8', xml_declaration=True)
edit('widgets/mainwindow.cpp', '    progressBar.setVisible(true);', '    progressBar.setVisible(m_transmitting);')

# Save an authentic widget rendering from the isolated startup smoke test.
edit('main.cpp', '                std::cout << "JTTY-only mode restriction passed" << std::endl;',
     '''                std::cout << "JTTY-only mode restriction passed" << std::endl;
                auto const preview = qEnvironmentVariable("JTTY_UI_PREVIEW");
                if (!preview.isEmpty()) w.grab().save(preview);''')

# The independent client must not inherit an upstream release-candidate cutoff.
p = source / 'main.cpp'
text = p.read_text()
start = text.index('      auto const prerelease_expiration =')
end = text.index('      auto const original_style_sheet', start)
text = text[:start] + text[end:]
start = text.index('          if (prerelease_notice_pending)')
end = text.index('#ifdef WSJT_ENABLE_LIVE_AUDIO_TEST', start)
text = text[:start] + text[end:]
p.write_text(text)

shutil.copy2(root/'native/qt-ui/JttyLayout.hpp',source/'widgets/JttyLayout.hpp')
edit('widgets/mainwindow.h', '  QString jttyOperatingMode() const { return m_mode; }', '  void installJttyLayout();\n  bool jttyLayoutSmoke();\n  QString jttyOperatingMode() const { return m_mode; }')
edit('widgets/mainwindow.cpp', '// this must be the last statement of constructor', '  installJttyLayout();\n\n// this must be the last statement of constructor')
p=source/'widgets/mainwindow.cpp';p.write_text(p.read_text()+'\n#include "JttyLayout.hpp"\n')
# The implementation declaration must be known when Qt instantiates findChild.
edit('main.cpp', '                std::cout << "JTTY-only mode restriction passed" << std::endl;', '''                std::cout << "JTTY-only mode restriction passed" << std::endl;
                if (!w.jttyLayoutSmoke()) {
                  std::cerr << "JTTY layout/docking/frequency controls failed" << std::endl;
                  a.exit(EXIT_FAILURE); return;
                }
                std::cout << "JTTY layout/docking/frequency controls passed" << std::endl;''')
edit('widgets/mainwindow.h', '  bool jttyLayoutSmoke();', '''  bool jttyLayoutSmoke();
  void saveJttyLayout();
#ifdef WSJT_ENABLE_LIVE_AUDIO_TEST
  qint64 jttyLayoutSubmitFixture(QString message,bool macro);
#endif''')
edit('widgets/mainwindow_settings.cpp', 'void MainWindow::writeSettings()\n{', 'void MainWindow::writeSettings()\n{\n  saveJttyLayout();')
# Exercise the real floating composer/Enter and F1 routes through the existing
# WAV loopback fixture (exactly two queued requests, one continuous playout).
edit('JttyTxLoopbackTestController.cpp', 'm_window->submitJttyText (contestExchangeMessage ())', 'm_window->jttyLayoutSubmitFixture (contestExchangeMessage (),false)')
edit('JttyTxLoopbackTestController.cpp', 'm_window->submitJttyText (\n    adjacentStructuredFramesMessage ())', 'm_window->jttyLayoutSubmitFixture (\n    adjacentStructuredFramesMessage (),true)')
# Capture before shutdown closes the embedded waterfall.
edit('widgets/mainwindow.cpp', 'void MainWindow::closeEvent(QCloseEvent * e)\n{', 'void MainWindow::closeEvent(QCloseEvent * e)\n{\n  auto const preview=qEnvironmentVariable("JTTY_UI_PREVIEW");\n  if(!preview.isEmpty()) grab().save(preview);')

# Stop the monitor before opening the error dialog; retain the selected name.
edit('widgets/mainwindow.h', '  void saveJttyLayout();', '  void saveJttyLayout();\n  void jttyNoteInputError(QString const&);\n  QString m_jttyInputName;\n  QString m_jttyInputError;')
edit('widgets/mainwindow_show_messages.cpp', 'void MainWindow::showSoundInError(const QString& errorMsg)\n{', 'void MainWindow::showSoundInError(const QString& errorMsg)\n{\n  jttyNoteInputError(errorMsg);')
edit('widgets/mainwindow_slots.cpp', 'void MainWindow::on_monitorButton_clicked (bool checked)\n{', '''void MainWindow::on_monitorButton_clicked (bool checked)
{
  if(checked && !m_jttyInputError.isEmpty() && m_config.audio_input_device().isNull()) {
    ui->monitorButton->setChecked(false);m_monitoring=false;return;
  }''')
