#!/usr/bin/env python3
"""Expose the decoder's latest accepted frame SNR without changing its decisions."""
import pathlib,sys
source=pathlib.Path(sys.argv[1])
def edit(name,old,new):
 p=source/name;s=p.read_text();assert old in s,(name,old[:80]);p.write_text(s.replace(old,new))
# Keep the original updates ABI for upstream tests and callers.
f='lib/jtty/rjtty_sub.f90'
edit(f,'subroutine jtty_get_updates(text_blocks,message_ids,frequencies,start_tsync,eom,count)', 'subroutine jtty_get_updates_snr(text_blocks,message_ids,frequencies,start_tsync,eom,count,snr_db)')
edit(f,'  real, intent(out)             :: frequencies(BATCH_SIZE)', '  real, intent(out)             :: snr_db(BATCH_SIZE)\n  real, intent(out)             :: frequencies(BATCH_SIZE)')
edit(f,'  frequencies=0.0','  snr_db=-999.0\n  frequencies=0.0')
edit(f,'     frequencies(i)=pending_updates(index)%f1','     snr_db(i)=pending_updates(index)%snrdb-20.0\n     frequencies(i)=pending_updates(index)%f1')
edit(f,'end subroutine jtty_get_updates','end subroutine jtty_get_updates_snr')
p=source/f
p.write_text(p.read_text()+'''
subroutine jtty_get_updates(text_blocks,message_ids,frequencies,start_tsync,eom,count)
  use iso_fortran_env, only: int64
  character(len=2400), intent(out) :: text_blocks
  integer(int64), intent(out) :: message_ids(30)
  real, intent(out) :: frequencies(30),start_tsync(30)
  logical*1, intent(out) :: eom(30)
  integer, intent(out) :: count
  real :: snr_db(30)
  call jtty_get_updates_snr(text_blocks,message_ids,frequencies,start_tsync,eom,count,snr_db)
end subroutine jtty_get_updates
''')
f='lib/jtty/jtty_mdecode.f90'
# Only assemblies and queued updates need additional storage; frames have snrdb already.
edit(f,'  type :: message_assembly','  type :: message_assembly\n     real :: snrdb = -979.0')
edit(f,'  type :: message_update','  type :: message_update\n     real :: snrdb = -979.0')
edit(f,'pending_updates(index)%f1=message%f1','pending_updates(index)%snrdb=message%snrdb\n      pending_updates(index)%f1=message%f1')
edit(f,'      message%f1=candidate%f1','      message%snrdb=candidate%snrdb\n      message%f1=candidate%f1')
edit(f,'      active_messages(index)%f1=candidate%f1','      active_messages(index)%snrdb=candidate%snrdb\n      active_messages(index)%f1=candidate%f1')
f='widgets/JttyMessages.hpp'
edit(f,'    bool complete {false};','    bool complete {false};\n    float snrDb {-999.f};')
edit(f,'            || known->complete != complete)', '            || known->complete != complete || known->snrDb != update.snrDb)')
edit(f,'        known->frequency = update.frequency;', '        known->frequency = update.frequency;\n        known->snrDb = update.snrDb;')
f='widgets/mainwindow.h'
edit(f,'    bool written {false};\n    DecodeOperatingContext context;', '    bool written {false};\n    DecodeOperatingContext context;\n    float snrDb {-999.f};')
# QSO lines have no context and must retain existing aggregate field ordering.
s=(source/f).read_text();start=s.index('  struct JttyQsoLine');end=s.index('  };',start);s=s[:end]+'    float snrDb {-999.f};\n'+s[end:];(source/f).write_text(s)
f='widgets/mainwindow_jtty.cpp'
edit(f,'  void jtty_get_updates_(', '  void jtty_get_updates_snr_(')
edit(f,'int* count, fortran_charlen_t);','int* count, float snr_db[], fortran_charlen_t);')
edit(f,'      std::array<float, jttyMaxUpdates> frequencies {};','      std::array<float, jttyMaxUpdates> snrDb {};\n      std::array<float, jttyMaxUpdates> frequencies {};')
edit(f,'      jtty_get_updates_(', '      jtty_get_updates_snr_(')
edit(f,'complete.data(), &updateCount,','complete.data(), &updateCount, snrDb.data(),')
edit(f,'sequenceStarts[i], complete[i]});','sequenceStarts[i], complete[i], snrDb[i]});')
edit(f,'          decodeLine.frequency = update.frequency;', '          decodeLine.frequency = update.frequency;\n          decodeLine.snrDb = update.snrDb;')
edit(f,'bool const frequencyChanged = known->frequency != update.frequency;', 'bool const frequencyChanged = known->frequency != update.frequency || known->snrDb != update.snrDb;')
edit(f,'          known->frequency = update.frequency;', '          known->frequency = update.frequency;\n          known->snrDb = update.snrDb;')
edit(f,'update.sequenceStart, messageStartUtc});', 'update.sequenceStart, messageStartUtc, update.snrDb});')
# Only display rows gain the column; log/fixture message formats remain stable.
edit(f,'line.frequency, Jtty::wrapMessage (line.text))','line.frequency, Jtty::wrapMessage (line.text), line.snrDb)')
edit(f,'QString formatJttyDecodeLine (float frequency, QString const& message)', 'QString formatJttyDecodeLine (float frequency, QString const& message, float snrDb = -999.f)')
# Insert SNR prefix only when supplied, preserving the existing frequency-first legacy path.
edit(f,'    return message.isEmpty()', '    if (snrDb > -900.f) return QStringLiteral("%1  ").arg(qRound(snrDb),3) + frequencyText + "  " + message;\n    return message.isEmpty()')
edit(f,'QStringLiteral("  UTC  Freq  ") : QStringLiteral("Freq  ")','QStringLiteral("  UTC   dB  Freq  ") : QStringLiteral(" dB  Freq  ")')

edit(f,'  ui->lh_decodes_headings_label->setText(prefix + tr ("Message"));', '  ui->lh_decodes_headings_label->setText(prefix + tr ("Message"));\n  ui->decodedTextBrowser->setToolTip(tr("dB: estimated SNR of the latest accepted JTTY frame"));\n  ui->decodedTextBrowser2->setToolTip(ui->decodedTextBrowser->toolTip());')
# A real synthetic audio fixture must deliver numeric SNR to both panes.
edit('LiveAudioTestController.cpp', '  if (m_expectedJtty.size () == 1', r'''  QRegularExpression const snrColumn {QStringLiteral("(?:^|\\n)\\s*(?:[0-9]{6}\\s+)?[-+]?[0-9]{1,3}\\s+[0-9]{3,4}\\s+")};
  if (!snrColumn.match(m_jttyAllDecodes->toPlainText()).hasMatch()
      || !snrColumn.match(m_jttyQsoFrequency->toPlainText()).hasMatch()) {
    fail(tr("JTTY SNR did not reach both message panes.")); return;
  }
  if (m_expectedJtty.size () == 1''')
