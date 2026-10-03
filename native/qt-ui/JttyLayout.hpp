// JTTY presentation adapter. Existing widgets retain their names and connections.
#include <QCloseEvent>
#include <QKeyEvent>
#include <QCheckBox>
#include <QDialog>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QTimer>
#include <QPushButton>
#include <QSpinBox>
#include <QProgressBar>
#include <functional>
#include <QProcess>
#include <QDir>
#include <QDateTime>
#include <QActionGroup>
#include <QMenu>

class JttyConversationWindow final : public QDialog {
public:
  explicit JttyConversationWindow(QWidget *parent) : QDialog(parent) {
    setWindowTitle("JTTY Workbench — Conversation");
    setModal(false);
    setAttribute(Qt::WA_QuitOnClose, false);
    resize(850, 620);
    setMinimumSize(620, 460);
    new QVBoxLayout(this);
  }
  std::function<void()> restore;
  std::function<bool(QKeyEvent*)> keyHandler;
protected:
  void keyPressEvent(QKeyEvent *event) override {
    if (keyHandler && keyHandler(event)) { event->accept(); return; }
    QDialog::keyPressEvent(event);
  }
  void closeEvent(QCloseEvent *event) override {
    if (restore) restore();
    event->accept();
  }
};

void MainWindow::installJttyLayout() {
  if (findChild<QWidget *>("jttyWorkbenchRoot")) return;
  auto original = takeCentralWidget();
  original->setParent(this);
  original->hide(); // Retain hidden mode widgets required by upstream logic.
  auto root = new QWidget(this);
  root->setObjectName("jttyWorkbenchRoot");
  auto outer = new QVBoxLayout(root);
  outer->setContentsMargins(16, 14, 16, 10);
  outer->setSpacing(14);
  auto panel = [](QWidget *parent, const char *name) {
    auto frame = new QFrame(parent);
    frame->setObjectName(name);
    frame->setProperty("jttyPanel", true);
    return frame;
  };
  auto label = [](QString text, QWidget *parent, bool heading = false) {
    auto result = new QLabel(text, parent);
    result->setProperty("jttyHeading", heading);
    result->setProperty("jttyFieldLabel", !heading);
    return result;
  };
  auto mount = [](QLayout *layout, QWidget *widget) {
    widget->setMinimumSize(0, 0);
    widget->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    layout->addWidget(widget);
    widget->show();
  };
  ui->menuMode->menuAction()->setVisible(false);
  auto top = panel(root, "jttyFrequencyStrip");
  auto frequencies = new QGridLayout(top);
  frequencies->setContentsMargins(16, 12, 16, 12);
  frequencies->setHorizontalSpacing(12);
  frequencies->setVerticalSpacing(8);
  frequencies->setColumnStretch(0,1);
  auto radioMode=label(tr("CAT mode unavailable"),top);
  radioMode->setObjectName("jttyRadioMode");
  frequencies->addWidget(radioMode,0,0);
  ui->labDialFreq->setParent(top); ui->labDialFreq->show();
  ui->labDialFreq->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
  ui->labDialFreq->setStyleSheet("background: transparent; font-size: 28px; font-weight: bold; border: none;");
  frequencies->addWidget(ui->labDialFreq,1,0);
  auto preset=new QHBoxLayout;
  preset->setSpacing(8);
  mount(preset,ui->bandComboBox); mount(preset,ui->readFreq);
  ui->readFreq->setFixedSize(28,28);
  ui->bandComboBox->setMinimumWidth(205);
  ui->bandComboBox->setFixedHeight(32);
  frequencies->addLayout(preset,2,0);
  frequencies->addWidget(label(tr("RECEIVE OFFSET · Hz"),top),0,1);
  frequencies->addWidget(label(tr("TRANSMIT OFFSET · Hz"),top),0,2);
  frequencies->addWidget(label(tr("RX TOLERANCE · ±Hz"),top),0,3);
  auto placeControl=[&](QWidget *widget,int row,int column) {
    widget->setMinimumSize(0,0);
    widget->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
    widget->setFixedHeight(32);
    widget->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    frequencies->addWidget(widget,row,column); widget->show();
  };
  placeControl(ui->RxFreqSpinBox_2,1,1);
  placeControl(ui->TxFreqSpinBox_2,1,2);
  placeControl(ui->sbFtol_2,1,3);
  ui->pbR2T_2->setText(tr("Copy Rx → Tx"));
  ui->pbT2R_2->setText(tr("Copy Tx → Rx"));
  placeControl(ui->pbR2T_2,2,1); placeControl(ui->pbT2R_2,2,2);
  for(int column=1;column<=3;++column) frequencies->setColumnMinimumWidth(column,140);
  // Keep action columns balanced and prevent tolerance absorbing spare width.
  ui->RxFreqSpinBox_2->setMaximumWidth(160);
  ui->TxFreqSpinBox_2->setMaximumWidth(160);
  ui->sbFtol_2->setMaximumWidth(160);
  ui->pbR2T_2->setMaximumWidth(160);ui->pbT2R_2->setMaximumWidth(160);
  auto cat=label(QString{},top);
  cat->setObjectName("jttyCatStatus");
  frequencies->addWidget(cat,0,4,1,3,Qt::AlignLeft|Qt::AlignVCenter);
  auto settings=new QPushButton(tr("Radio / Audio…"),top);
  settings->setObjectName("jttySettingsButton");
  connect(settings,&QPushButton::clicked,this,&MainWindow::on_actionSettings_triggered);
  placeControl(settings,1,4);
  placeControl(ui->monitorButton,1,5); placeControl(ui->stopTxButton,1,6);
  placeControl(ui->tuneButton,2,5); placeControl(ui->logQSOButton,2,6);
  for(auto widget : {settings,ui->monitorButton,ui->stopTxButton,ui->tuneButton,ui->logQSOButton}) {
    widget->setMinimumWidth(110);widget->setMaximumWidth(140);
  }
  outer->addWidget(top);

  auto vertical = new QSplitter(Qt::Vertical, root);
  vertical->setObjectName("jttyVerticalSplitter");
  auto messages = new QSplitter(Qt::Horizontal, vertical);
  messages->setObjectName("jttyMessagesSplitter");
  auto received = panel(messages, "jttyReceivedPanel");
  received->setMinimumWidth(295);
  auto receivedLayout = new QVBoxLayout(received);
  receivedLayout->setContentsMargins(14, 12, 14, 12);
  auto receivedHeader = new QHBoxLayout;
  mount(receivedHeader, ui->lh_decodes_title_label);
  ui->lh_decodes_title_label->setText(tr("Received messages"));
  auto clearReceived = new QPushButton(tr("Clear"), received);
  clearReceived->setObjectName("jttyClearReceived");
  clearReceived->setFixedWidth(70);
  receivedHeader->setStretch(0,1);
  receivedHeader->addWidget(clearReceived);
  connect(clearReceived, &QPushButton::clicked, this, [this] {
    flushJttyDecodeLines();
    m_jttyAllFreqLines.clear();
    m_jttyAllFreqsGroupStart = QTextBlock();
    ui->decodedTextBrowser->erase();
  });
  receivedLayout->addLayout(receivedHeader);
  receivedLayout->addWidget(label(tr("All Messages"), received));
  mount(receivedLayout, ui->lh_decodes_headings_label);
  mount(receivedLayout, ui->decodedTextBrowser);
  receivedLayout->setStretch(receivedLayout->count()-1, 1);
  ui->decodedTextBrowser->setMinimumHeight(60);

  auto conversation = panel(messages, "jttyConversationPanel");
  conversation->setMinimumHeight(0);
  conversation->setMinimumWidth(620);
  auto conversationLayout = new QVBoxLayout(conversation);
  conversationLayout->setContentsMargins(14, 12, 14, 12);
  auto conversationHeader = new QHBoxLayout;
  mount(conversationHeader, ui->rh_decodes_title_label);
  ui->rh_decodes_title_label->setText(tr("Conversation"));
  auto popout = new QPushButton(tr("Pop out ↗"), conversation);
  popout->setObjectName("jttyPopoutConversation");
  auto clearConversation = new QPushButton(tr("Clear"), conversation);
  clearConversation->setObjectName("jttyClearConversation");
  popout->setFixedWidth(112);
  clearConversation->setFixedWidth(70);
  conversationHeader->setStretch(0,1);
  conversationHeader->addWidget(popout);
  conversationHeader->addWidget(clearConversation);
  conversationLayout->addLayout(conversationHeader);
  auto scope = label(QString{}, conversation);
  scope->setObjectName("jttyConversationScope");
  conversationLayout->addWidget(scope);
  connect(clearConversation, &QPushButton::clicked, this, [this] {
    ui->decodedTextBrowser2->erase(); // Upstream erased callback resets QSO history.
    m_jttyQsoGroupStart = QTextBlock();
    m_jttyQsoGroupEnd = QTextBlock();
  });
  mount(conversationLayout, ui->decodedTextBrowser2);
  conversationLayout->setStretch(conversationLayout->count()-1, 1);
  ui->decodedTextBrowser2->setMinimumHeight(60);
  auto draftLabel = new QHBoxLayout;
  draftLabel->addWidget(label(tr("Message"), conversation));
  draftLabel->addStretch();
  auto counter = label(QString{}, conversation);
  counter->setObjectName("jttyDraftCount");
  draftLabel->addWidget(counter);
  conversationLayout->addLayout(draftLabel);
  auto draft = new QHBoxLayout;
  mount(draft, ui->Tx_Message);
  ui->Tx_Message->setMinimumHeight(36);
  draft->setStretch(0, 1);
  mount(draft, ui->pbSendMessage);
  conversationLayout->addLayout(draft);
  auto macros = new QGridLayout;
  QPushButton *buttons[] = {ui->pbF1,ui->pbF2,ui->pbF3,ui->pbF4,ui->pbF5,ui->pbF6,ui->pbF7,ui->pbF8};
  QLineEdit *templates[] = {ui->msg1,ui->msg2,ui->msg3,ui->msg4,ui->msg5,ui->msg6,ui->msg7,ui->msg8};
  for(int i=0;i<8;++i) {
    for(auto widget : {static_cast<QWidget*>(buttons[i]),static_cast<QWidget*>(templates[i])}) {
      widget->setMinimumSize(0,0); widget->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX); widget->show();
    }
    macros->addWidget(buttons[i],0,i);
    macros->addWidget(templates[i],1,i);
  }
  conversationLayout->addLayout(macros);
  auto contacts = new QHBoxLayout;
  contacts->addWidget(label(tr("Their call"), conversation));
  mount(contacts,ui->dxCallEntry);
  ui->dxCallEntry->setClearButtonEnabled(true);
  contacts->addWidget(label(tr("Call next"), conversation));
  mount(contacts,ui->lineEdit);
  ui->lineEdit->setClearButtonEnabled(true);
  contacts->addWidget(label(tr("Serial"), conversation));
  mount(contacts,ui->sbSerialNumber_2);
  conversationLayout->addLayout(contacts);
  auto options = new QHBoxLayout;
  mount(options,ui->cbLowerCase);
  mount(options,ui->cbIncludeTime);
  options->addStretch();
  mount(options,ui->DX_Call_Button);
  mount(options,ui->dxGridEntry);
  ui->dxGridEntry->setMaximumWidth(100);
  conversationLayout->addLayout(options);
  auto tools = new QHBoxLayout;
  mount(tools,ui->lookupButton); mount(tools,ui->addButton); mount(tools,ui->ignoreButton);
  tools->addStretch(); mount(tools,ui->stopButton);
  conversationLayout->addLayout(tools);

  auto floating = new JttyConversationWindow(this);
  floating->setObjectName("jttyFloatingConversation");
  auto dockBack = [this,messages,conversation,popout,floating] {
    m_settings->setValue("MainWindow/JttyConversationGeometry",floating->saveGeometry());
    messages->addWidget(conversation);
    conversation->show();
    messages->setSizes({380,820});
    popout->setText(tr("Pop out ↗"));
    floating->hide();
    ui->Tx_Message->setFocus();
  };
  floating->restoreGeometry(m_settings->value("MainWindow/JttyConversationGeometry").toByteArray());
  floating->restore = dockBack;
  floating->keyHandler = [this](QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape || (event->key() >= Qt::Key_F1 && event->key() <= Qt::Key_F8)) {
      jtty_key_struck(event); return true;
    }
    return false;
  };
  connect(popout,&QPushButton::clicked,this,[this,conversation,popout,floating,dockBack] {
    if (floating->isVisible()) { dockBack(); return; }
    floating->layout()->addWidget(conversation);
    popout->setText(tr("Dock back ↙"));
    conversation->show(); floating->show(); floating->raise();
    ui->Tx_Message->setFocus();
  });
  messages->setCollapsible(0,false); messages->setCollapsible(1,false);
  messages->setSizes({380,820});

  auto waterfall = panel(vertical,"jttyWaterfallPanel");
  auto waterfallLayout = new QVBoxLayout(waterfall);
  waterfallLayout->setContentsMargins(12,8,12,8);
  auto waterfallHeader = new QHBoxLayout;
  waterfallHeader->addWidget(label(tr("Waterfall — click a signal to set Rx"),waterfall,true));
  waterfallHeader->addStretch();
  auto displayControls = new QPushButton(tr("Display controls"),waterfall);
  displayControls->setObjectName("jttyWaterfallControls");
  displayControls->setCheckable(true);
  auto waterfallControls = m_wideGraph->findChild<QCheckBox*>("cbControls");
  if (waterfallControls) {
    waterfallControls->hide();
    waterfallControls->setChecked(m_settings->value("MainWindow/JttyWaterfallControls",false).toBool());
    displayControls->setChecked(waterfallControls->isChecked());
    connect(displayControls,&QPushButton::toggled,waterfallControls,&QCheckBox::setChecked);
    connect(waterfallControls,&QCheckBox::toggled,displayControls,&QPushButton::setChecked);
  }
  waterfallHeader->addWidget(displayControls);
  waterfallHeader->addWidget(label(tr("Input"),waterfall));
  auto reading = ui->signal_meter_widget->findChild<QLabel*>();
  auto meter = new QProgressBar(waterfall);
  meter->setRange(0,90); meter->setTextVisible(false); meter->setFixedSize(100,12);
  waterfallHeader->addWidget(meter);
  if (reading) {
    mount(waterfallHeader,reading);
    reading->setFixedWidth(reading->fontMetrics().horizontalAdvance("−100 dB")+12);
    reading->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
  }
  ui->signal_meter_widget->hide();
  waterfallHeader->addWidget(label(tr("Tx attenuation"),waterfall));
  mount(waterfallHeader,ui->outAttenuation);
  ui->outAttenuation->setOrientation(Qt::Horizontal);
  ui->outAttenuation->setFixedWidth(100);
  mount(waterfallHeader,ui->label);
  waterfallLayout->addLayout(waterfallHeader);
  m_wideGraph->setParent(waterfall, Qt::Widget);
  m_wideGraph->setMinimumSize(0,80);
  m_wideGraph->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
  waterfallLayout->addWidget(m_wideGraph.data());
  m_wideGraph->show();
  vertical->setCollapsible(0,false); vertical->setCollapsible(1,false);
  messages->setMinimumHeight(0);
  vertical->setSizes({580,240});
  outer->addWidget(vertical,1);
  auto devices = label(QString{},root);
  devices->setObjectName("jttyDeviceStatus");
  outer->addWidget(devices);
  m_jttyInputName=m_config.audio_input_device().deviceName();
  connect(this,&MainWindow::startAudioInputStream,this,[this](QAudioDeviceInfo const& device){
    m_jttyInputName=device.deviceName();m_jttyInputError.clear();
    setProperty("jttyAudioSystemTraceRequested",false);
  });
  connect(m_soundInput,&AudioInputSource::error,this,&MainWindow::jttyNoteInputError);
  auto update = [this,cat,scope,devices,counter,reading,meter,radioMode] {
    QString mode;
    if(m_config.rig_name()!="None" && m_config.is_transceiver_online()) {
      switch(m_rigState.mode()) {
        case Transceiver::CW: mode="CW"; break;
        case Transceiver::CW_R: mode="CW-R"; break;
        case Transceiver::USB: mode="USB"; break;
        case Transceiver::LSB: mode="LSB"; break;
        case Transceiver::FSK: mode="RTTY"; break;
        case Transceiver::FSK_R: mode="RTTY-R"; break;
        case Transceiver::DIG_U: mode="DATA-USB"; break;
        case Transceiver::DIG_L: mode="DATA-LSB"; break;
        case Transceiver::AM: mode="AM"; break;
        case Transceiver::FM: mode="FM"; break;
        case Transceiver::DIG_FM: mode="DATA-FM"; break;
        default: break;
      }
    }
    radioMode->setText(mode.isEmpty() ? tr("CAT mode unavailable") : mode);
    ui->rh_decodes_title_label->setText(tr("Conversation"));
    if (reading) meter->setValue(reading->text().section(' ',0,0).toInt());
    bool const connected=m_config.rig_name()!="None" && m_config.is_transceiver_online();
    cat->setTextFormat(Qt::RichText);
    cat->setText(QStringLiteral("<span style='color:%1'>●</span> %2")
      .arg(connected ? "#27b85c" : "#89929d",
           m_config.rig_name()=="None" ? tr("Radio not selected") : (connected ? tr("CAT connected") : tr("CAT disconnected"))));
    scope->setText(tr("Rx %1 Hz · ±%2 Hz · traffic near this frequency").arg(ui->RxFreqSpinBox_2->value()).arg(ui->sbFtol_2->value()));
    counter->setText(tr("%1 / 80 characters").arg(ui->Tx_Message->text().size()));
    devices->setText(tr("RX · %1   |   OUT · %2   |   %3").arg(!m_jttyInputError.isEmpty() ? tr("%1 (unavailable)").arg(m_jttyInputName.isEmpty()?tr("Input"):m_jttyInputName) : (m_config.audio_input_device().isNull() ? tr("No input selected") : m_config.audio_input_device().deviceName()),m_config.audio_output_device().isNull() ? tr("No output selected") : m_config.audio_output_device().deviceName(),!m_jttyInputError.isEmpty() ? tr("Input error · Monitor stopped") : (m_transmitting ? tr("Transmission active / queued") : (m_monitoring ? tr("Monitoring") : tr("Monitor stopped")))));
  };
  auto timer = new QTimer(root); timer->setInterval(250);
  connect(timer,&QTimer::timeout,this,update); timer->start(); update();
  setCentralWidget(root);
  auto themes=ui->menuView->addMenu(tr("Themes"));
  auto themeGroup=new QActionGroup(this);
  themeGroup->setExclusive(true);
  for(bool dark : {false,true}) {
    auto action=themes->addAction(dark ? tr("Dark") : tr("Light gray"));
    action->setObjectName(dark ? "jttyDarkTheme" : "jttyLightTheme");
    action->setCheckable(true); themeGroup->addAction(action);
    action->setChecked(m_settings->value("MainWindow/JttyDarkTheme",false).toBool()==dark);
    connect(action,&QAction::triggered,this,[this,dark] {
      m_settings->setValue("MainWindow/JttyDarkTheme",dark);
      applyApplicationStyle(qApp->font(),dark);
    });
  }
  applyApplicationStyle(qApp->font(),m_settings->value("MainWindow/JttyDarkTheme",false).toBool());
  auto fontMenu=ui->menuView->addMenu(tr("Message font size"));
  auto fontGroup=new QActionGroup(this);
  fontGroup->setObjectName("jttyFontSizes"); fontGroup->setExclusive(true);
  int const selectedSize=qBound(8,m_settings->value("MainWindow/JttyMessageFontSize",m_settings->value("MainWindow/JttyFontSize",11)).toInt(),17);
  for(int size=8;size<=17;++size) {
    auto action=fontMenu->addAction(tr("%1 pt").arg(size));
    action->setObjectName(QString("jttyFontSize%1").arg(size));
    action->setData(size); action->setCheckable(true); fontGroup->addAction(action);
    action->setChecked(size==selectedSize);
    connect(action,&QAction::triggered,this,[this,size] {
      m_settings->setValue("MainWindow/JttyMessageFontSize",size);
      QFont font=ui->decodedTextBrowser->contentFont();font.setPointSize(size);
      ui->decodedTextBrowser->setContentFont(font);
      ui->decodedTextBrowser2->setContentFont(font);
      ui->Tx_Message->setFont(font);
      ui->Tx_Message->setStyleSheet(QString("font-size: %1pt;").arg(size));
    });
  }
  setMinimumSize(1080,720);
  resize(1280,1060);
  restoreGeometry(m_settings->value("MainWindow/JttyLayoutGeometry").toByteArray());
  messages->restoreState(m_settings->value("MainWindow/JttyMessagesSplitter").toByteArray());
  vertical->restoreState(m_settings->value("MainWindow/JttyVerticalSplitter").toByteArray());
  root->show();
}

void MainWindow::jttyNoteInputError(QString const& message) {
  if(!m_config.audio_input_device().isNull()) m_jttyInputName=m_config.audio_input_device().deviceName();
  m_jttyInputError=message;
#ifdef Q_OS_MAC
  if(!m_automated_test && !property("jttyAudioSystemTraceRequested").toBool()) {
    setProperty("jttyAudioSystemTraceRequested",true);
    auto folder=QDir::homePath()+"/Library/Application Support/JTTY Workbench/Logs";
    QDir().mkpath(folder);
    auto path=folder+"/coreaudio-failure-"+QDateTime::currentDateTimeUtc().toString("yyyyMMdd-HHmmss-zzz")+".log";
    auto collector=new QProcess(this);
    collector->setProcessChannelMode(QProcess::MergedChannels);collector->setStandardOutputFile(path);
    connect(collector,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),collector,[collector,path](int code,QProcess::ExitStatus){
      LOG_INFO("JTTY CoreAudio system trace saved path=" << path.toStdString() << " collector_exit=" << code);collector->deleteLater();
    });
    connect(collector,&QProcess::errorOccurred,collector,[collector,path](QProcess::ProcessError){
      LOG_WARN("JTTY CoreAudio trace collector error path=" << path.toStdString() << " error=" << collector->errorString().toStdString());collector->deleteLater();
    });
    collector->start("/usr/bin/log",{"show","--last","90s","--style","compact","--predicate","process == \"coreaudiod\" OR process == \"audiomxd\""});
    LOG_INFO("JTTY capture failure=" << message.toStdString() << " automatic_system_trace=" << path.toStdString());
  }
#endif
  if(m_monitoring) on_monitorButton_clicked(false);
  ui->monitorButton->setChecked(false);
}

bool MainWindow::jttyLayoutSmoke() {
#ifdef WSJT_ENABLE_LIVE_AUDIO_TEST
  // Actual stop-monitor route with a synthetic three-second capture, no input
  // stream or transmitter. Verify selective saving and valid WAV output.
  auto savedK=m_k0; auto savedLast=m_jttyLastSavedWavK0;
  auto savedDecoded=m_bDecoded; auto savedAll=m_saveAll; auto savedSelective=m_saveDecoded;
  auto savedDisk=m_diskData; auto savedMonitoring=m_monitoring; auto savedFilename=m_fnameWE;
  m_k0=36000; m_jttyLastSavedWavK0=-1;
  m_saveAll=false; m_saveDecoded=true; m_diskData=false; m_bDecoded=false;
  jtty_save_wav();
  if(m_jttyLastSavedWavK0!=-1) return false;
  m_bDecoded=true; m_monitoring=true;
  std::fill(dec_data.d2,dec_data.d2+36000,short(123));
  on_monitorButton_clicked(false);
  m_saveWAVSynchronizer.waitForFinished();
  QFile recording(m_fnameWE+".wav");
  bool valid=m_jttyLastSavedWavK0==36000 && recording.open(QIODevice::ReadOnly)
    && recording.read(4)=="RIFF" && recording.size()>=72044;
  recording.close(); recording.remove();
  m_k0=savedK; m_jttyLastSavedWavK0=savedLast; m_bDecoded=savedDecoded;
  m_saveAll=savedAll; m_saveDecoded=savedSelective; m_diskData=savedDisk;
  m_monitoring=savedMonitoring; m_fnameWE=savedFilename;
  if(!valid) return false;
#endif
  auto messages=findChild<QSplitter*>("jttyMessagesSplitter");
  auto conversation=findChild<QWidget*>("jttyConversationPanel");
  auto floating=findChild<QDialog*>("jttyFloatingConversation");
  auto popout=findChild<QPushButton*>("jttyPopoutConversation");
  if(!messages || !conversation || !floating || !popout) return false;
  if(!ui->dxCallEntry->isClearButtonEnabled() || !ui->lineEdit->isClearButtonEnabled()) return false;
  auto fontGroup=findChild<QActionGroup*>("jttyFontSizes");
  if(!fontGroup || fontGroup->actions().size()!=10 || !fontGroup->checkedAction()) return false;
  auto savedSize=fontGroup->checkedAction();
  auto savedText=ui->decodedTextBrowser->toPlainText();
  auto originalUiFont=qApp->font();
  auto originalButtonFont=ui->monitorButton->font();
  for(int size : {8,17}) {
    auto action=findChild<QAction*>(QString("jttyFontSize%1").arg(size));
    if(!action) return false;
    action->trigger();
    if(qApp->font()!=originalUiFont || ui->monitorButton->font()!=originalButtonFont
       || ui->decodedTextBrowser->contentFont().pointSize()!=size
       || ui->decodedTextBrowser2->contentFont().pointSize()!=size
       || ui->Tx_Message->font().pointSize()!=size
       || ui->decodedTextBrowser->toPlainText()!=savedText) return false;
  }
  savedSize->trigger();
  auto darkTheme=findChild<QAction*>("jttyDarkTheme");
  auto lightTheme=findChild<QAction*>("jttyLightTheme");
  auto controls=findChild<QPushButton*>("jttyWaterfallControls");
  auto checkbox=m_wideGraph->findChild<QCheckBox*>("cbControls");
  auto controlPanel=m_wideGraph->findChild<QWidget*>("controls_widget");
  if(!darkTheme || !lightTheme || !controls || !checkbox || !controlPanel || !checkbox->isHidden()) return false;
  bool savedDark=m_settings->value("MainWindow/JttyDarkTheme",false).toBool();
  darkTheme->trigger();
  if(!darkTheme->isChecked() || lightTheme->isChecked() || !qApp->styleSheet().contains("#262a30")) return false;
  lightTheme->trigger();
  if(!lightTheme->isChecked() || darkTheme->isChecked() || !qApp->styleSheet().contains("#d0d2d5")) return false;
  (savedDark ? darkTheme : lightTheme)->trigger();
  bool savedControls=controls->isChecked();
  controls->setChecked(true);
  if(!checkbox->isChecked() || controlPanel->isHidden()) return false;
  controls->setChecked(false);
  if(checkbox->isChecked() || !controlPanel->isHidden()) return false;
  controls->setChecked(savedControls);
  auto originalSize=size();
  resize(width(),820);
  QApplication::processEvents();
  bool shrunk=height()<=820;
  auto toolsButton=ui->lookupButton;
  bool accessible=conversation->rect().contains(toolsButton->mapTo(conversation,QPoint(0,0)))
    && conversation->rect().contains(toolsButton->mapTo(conversation,QPoint(toolsButton->width()-1,toolsButton->height()-1)));
  resize(width(),1160);
  QApplication::processEvents();
  bool grew=height()>=1160;
  resize(originalSize);
  if(!shrunk || !grew || !accessible) return false;
  auto input=m_soundInput;
  auto draft=ui->Tx_Message; auto transcript=ui->decodedTextBrowser2;
  auto originalRx=ui->RxFreqSpinBox_2->value(); auto originalTx=ui->TxFreqSpinBox_2->value();
  auto originalCall=ui->dxCallEntry->text();
  ui->TxFreqSpinBox_2->setValue(1600);
  Q_EMIT ui->decodedTextBrowser->selectCallsign(" 1234  CQ N4EAC", "N4EAC", Qt::NoModifier);
  if(ui->RxFreqSpinBox_2->value()!=1234 || m_wideGraph->rxFreq()!=1234
     || ui->TxFreqSpinBox_2->value()!=1600 || ui->dxCallEntry->text()!="N4EAC") return false;
  Q_EMIT ui->decodedTextBrowser->selectCallsign("093318  1456  CQ K1ABC", "1456", Qt::NoModifier);
  if(ui->RxFreqSpinBox_2->value()!=1456 || m_wideGraph->rxFreq()!=1456
     || ui->TxFreqSpinBox_2->value()!=1600 || ui->dxCallEntry->text()!="N4EAC") return false;
  Q_EMIT ui->decodedTextBrowser2->selectCallsign("093318  1456  CQ k1abc/p", "k1abc/p", Qt::NoModifier);
  if(ui->dxCallEntry->text()!="K1ABC/P" || ui->TxFreqSpinBox_2->value()!=1600) return false;
  Q_EMIT ui->decodedTextBrowser->selectCallsign(" 1456  CQ K1ABC", "CQ", Qt::NoModifier);
  if(ui->dxCallEntry->text()!="K1ABC/P") return false;
  Q_EMIT ui->decodedTextBrowser->selectCallsign("181721  -7  1501  DE K6LJ", "-7", Qt::NoModifier);
  if(ui->RxFreqSpinBox_2->value()!=1501 || m_wideGraph->rxFreq()!=1501
     || ui->TxFreqSpinBox_2->value()!=1600 || ui->dxCallEntry->text()!="K1ABC/P") return false;
  ui->dxCallEntry->setText(originalCall);
  ui->RxFreqSpinBox_2->setValue(1400);
  ui->pbR2T_2->click();
  if(ui->TxFreqSpinBox_2->value()!=1400) return false;
  ui->TxFreqSpinBox_2->setValue(1600);
  ui->pbT2R_2->click();
  if(ui->RxFreqSpinBox_2->value()!=1600) return false;
  ui->RxFreqSpinBox_2->setValue(originalRx); ui->TxFreqSpinBox_2->setValue(originalTx);
  auto savedDraft=draft->text(); draft->setText("UNSENT LAYOUT CHECK");
  auto originalHistory=transcript->toPlainText();
  auto originalTolerance=ui->sbFtol_2->value();
  ui->sbFtol_2->setValue(100);
  bool tolerance=ui->sbFtol_2->value()==100;
  ui->sbFtol_2->setValue(originalTolerance);
  int opens=0;
  auto connection=connect(this,&MainWindow::startAudioInputStream,this,[&opens]{++opens;});
  popout->click();
  auto floated=floating->isVisible() && conversation->parentWidget()==floating;
  auto preview=qEnvironmentVariable("JTTY_UI_PREVIEW");
  if(!preview.isEmpty()) floating->grab().save(preview+".floating.png");
  auto actualFloating=static_cast<JttyConversationWindow*>(floating);
  auto realHandler=actualFloating->keyHandler;
  int functionKeys=0;
  actualFloating->keyHandler=[&functionKeys](QKeyEvent *event){ if(event->key()==Qt::Key_F1){++functionKeys;return true;}return false;};
  QKeyEvent functionKey(QEvent::KeyPress,Qt::Key_F1,Qt::NoModifier);
  QApplication::sendEvent(draft,&functionKey);
  actualFloating->keyHandler=realHandler;
  floating->close();
  auto restored=conversation->parentWidget()==messages && !floating->isVisible();
  disconnect(connection);
  bool preserved=draft==ui->Tx_Message && transcript==ui->decodedTextBrowser2 && m_soundInput==input && draft->text()=="UNSENT LAYOUT CHECK" && transcript->toPlainText()==originalHistory && opens==0 && tolerance && functionKeys==1;
  draft->setText(savedDraft);
  auto wasMonitoring=m_monitoring;
  jttyNoteInputError(QStringLiteral("Simulated capture failure"));
  if(m_monitoring || ui->monitorButton->isChecked() || m_jttyInputError.isEmpty()) return false;
  on_monitorButton_clicked(true);
  if(m_config.audio_input_device().isNull() && m_monitoring) return false;
  m_jttyInputError.clear();
  if(wasMonitoring) on_monitorButton_clicked(true);
  return floated && restored && preserved;
}

void MainWindow::saveJttyLayout() {
  auto vertical=findChild<QSplitter*>("jttyVerticalSplitter");
  auto messages=findChild<QSplitter*>("jttyMessagesSplitter");
  if(!vertical || !messages) return;
  m_settings->setValue("MainWindow/JttyLayoutGeometry",saveGeometry());
  m_settings->setValue("MainWindow/JttyVerticalSplitter",vertical->saveState());
  if(messages->count()==2) m_settings->setValue("MainWindow/JttyMessagesSplitter",messages->saveState());
  auto controls=findChild<QPushButton*>("jttyWaterfallControls");
  if(controls) m_settings->setValue("MainWindow/JttyWaterfallControls",controls->isChecked());
}

#ifdef WSJT_ENABLE_LIVE_AUDIO_TEST
qint64 MainWindow::jttyLayoutSubmitFixture(QString message,bool macro) {
  // Called only by the upstream WAV-output fixture controller, never hardware.
  if(!m_automated_test) return -1;
  auto popout=findChild<QPushButton*>("jttyPopoutConversation");
  auto floating=findChild<QDialog*>("jttyFloatingConversation");
  if(!popout || !floating) return -1;
  qint64 id=-1; int accepted=0;
  auto connection=connect(this,&MainWindow::jttyTextAccepted,this,[&](qint64 request){id=request;++accepted;});
  if(!floating->isVisible()) popout->click();
  if(macro) {
    auto saved=ui->msg1->text(); ui->msg1->setText(message);
    QKeyEvent key(QEvent::KeyPress,Qt::Key_F1,Qt::NoModifier);
    QApplication::sendEvent(ui->Tx_Message,&key);
    ui->msg1->setText(saved);
  } else {
    ui->Tx_Message->setText(message);
    QKeyEvent key(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier);
    QApplication::sendEvent(ui->Tx_Message,&key);
  }
  disconnect(connection);
  floating->close();
  return accepted==1 ? id : -1;
}
#endif
