#!/usr/bin/env python3
"""Apply receive and GUI lifecycle diagnostics to the prepared JTTY fork."""
from pathlib import Path
import sys
source=Path(sys.argv[1])
def edit(name,old,new):
 p=source/name;s=p.read_text();assert old in s,(name,old[:90]);p.write_text(s.replace(old,new))
edit('Detector/Detector.hpp','  unsigned m_frameRate;', '''  qint64 m_jttyDebugAt {0};
  quint64 m_jttyDebugFrames {0};
  std::array<double,2> m_jttyDebugEnergy {},m_jttyDebugPeak {};
  unsigned m_frameRate;''')
edit('Detector/Detector.cpp','#include "commons.h"','#include "commons.h"\n#include "Logger.hpp"\n#include <cstring>')
edit('Detector/Detector.cpp','  qint64 const frames_received = maxSize / bytes_per_frame;', '''  qint64 const frames_received = maxSize / bytes_per_frame;
  // Aggregate PCM statistics only; never save samples or do per-sample logging.
  auto channels=qMin(2,int(bytes_per_frame/sizeof(qint16)));
  for(qint64 frame=0;frame<frames_received;++frame) for(int channel=0;channel<channels;++channel) {
    qint16 raw=0;std::memcpy(&raw,data+frame*bytes_per_frame+channel*sizeof(qint16),sizeof(raw));
    double value=raw/32768.;m_jttyDebugEnergy[channel]+=value*value;
    m_jttyDebugPeak[channel]=std::max(m_jttyDebugPeak[channel],std::abs(value));
  }
  m_jttyDebugFrames+=frames_received;
  auto debugNow=QDateTime::currentMSecsSinceEpoch();
  if(debugNow-m_jttyDebugAt>=1000) {
    for(int channel=0;channel<channels;++channel) {
      auto rms=m_jttyDebugFrames?std::sqrt(m_jttyDebugEnergy[channel]/m_jttyDebugFrames):0.;
      LOG_INFO("JTTY PCM input channel=" << channel << " frames=" << m_jttyDebugFrames << " buffer_bytes=" << maxSize
        << " rms_dBFS=" << (rms>0?20*std::log10(rms):-160.)
        << " peak_dBFS=" << (m_jttyDebugPeak[channel]>0?20*std::log10(m_jttyDebugPeak[channel]):-160.)
        << " decode_input_blocked=" << dec_data_input_blocked());
    }
    m_jttyDebugFrames=0;m_jttyDebugEnergy.fill(0);m_jttyDebugPeak.fill(0);m_jttyDebugAt=debugNow;
  }''')
edit('Audio/soundin.cpp','  m_stream.reset (new QAudioInput {selected, format});', '''  LOG_INFO("JTTY Qt capture format preferred_rate=" << selected.preferredFormat().sampleRate()
    << " requested_rate=" << format.sampleRate() << " channels=" << format.channelCount() << " bits=" << format.sampleSize()
    << " type=" << int(format.sampleType()) << " endian=" << int(format.byteOrder()) << " frames_per_buffer=" << framesPerBuffer);
  m_stream.reset (new QAudioInput {selected, format});''')
edit('Audio/soundin.cpp','      m_stream->start (sink);','''      LOG_INFO("JTTY Qt capture starting sink=" << sink << " buffer_bytes=" << m_stream->bufferSize());
      m_stream->start (sink);
      LOG_INFO("JTTY Qt capture start returned state=" << int(m_stream->state()) << " error=" << int(m_stream->error())
        << " actual_rate=" << m_stream->format().sampleRate() << " period_bytes=" << m_stream->periodSize());''')
edit('widgets/mainwindow.cpp','  connect (this, &MainWindow::startAudioInputStream, m_soundInput, &AudioInputSource::start);','''  connect(this,&MainWindow::startAudioInputStream,this,[this](QAudioDeviceInfo const& device){
    LOG_INFO("JTTY GUI capture start request device=" << device.deviceName().toStdString() << " mode=" << m_mode.toStdString()
      << " monitoring=" << m_monitoring << " transmitting=" << m_transmitting << " CAT_online=" << m_config.is_transceiver_online());
  });
  connect (this, &MainWindow::startAudioInputStream, m_soundInput, &AudioInputSource::start);''')
edit('widgets/mainwindow.cpp','void MainWindow::monitor (bool state)\n{','''void MainWindow::monitor (bool state)
{
  LOG_INFO("JTTY GUI monitor transition requested=" << state << " previous=" << m_monitoring
    << " transmitting=" << m_transmitting << " CAT_online=" << m_config.is_transceiver_online()
    << " selected_input=" << m_config.audio_input_device().deviceName().toStdString());''')
edit('widgets/mainwindow.cpp','  inSettings = true;','''  LOG_INFO("JTTY GUI Settings opened input=" << m_config.audio_input_device().deviceName().toStdString()
    << " output=" << m_config.audio_output_device().deviceName().toStdString() << " monitoring=" << m_monitoring);
  inSettings = true;''')
edit('widgets/mainwindow.cpp','  if (QDialog::Accepted == m_config.exec ()) {','''  if (QDialog::Accepted == m_config.exec ()) {
    LOG_INFO("JTTY GUI Settings accepted input=" << m_config.audio_input_device().deviceName().toStdString()
      << " output=" << m_config.audio_output_device().deviceName().toStdString() << " rig=" << m_config.rig_name().toStdString()
      << " restart_input=" << m_config.restart_audio_input() << " restart_output=" << m_config.restart_audio_output());''')
edit('widgets/mainwindow.cpp','  Transceiver::TransceiverState old_state {m_rigState};','''  if(s.online()!=m_rigState.online() || s.ptt()!=m_rigState.ptt() || s.frequency()!=m_rigState.frequency())
    LOG_INFO("JTTY CAT transition online=" << s.online() << " PTT=" << s.ptt() << " frequency=" << s.frequency()
      << " monitoring=" << m_monitoring << " input=" << m_config.audio_input_device().deviceName().toStdString());
  Transceiver::TransceiverState old_state {m_rigState};''')
edit('Configuration.cpp','  m_->audio_input_device_ = QAudioDeviceInfo {};','''  LOG_INFO("JTTY configuration invalidating input=" << m_->audio_input_device_.deviceName().toStdString());
  m_->audio_input_device_ = QAudioDeviceInfo {};''')

edit('Audio/soundin.cpp','SoundInput::SoundInput(QObject *parent):AudioInputSource{parent},cummulative_lost_usec_{std::numeric_limits<qint64>::min()}{ }','SoundInput::SoundInput(QObject *parent):AudioInputSource{parent},cummulative_lost_usec_{std::numeric_limits<qint64>::min()}{LOG_INFO("JTTY deep audio diagnostics v1 Qt=" << qVersion());}')
