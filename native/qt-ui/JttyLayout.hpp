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
  auto top = panel(root, "jttyFrequencyStrip");
  auto frequencies = new QGridLayout(top);
  frequencies->setContentsMargins(16, 12, 16, 12);
  frequencies->setHorizontalSpacing(16);
  auto dial = new QVBoxLayout;
  dial->addWidget(label(tr("RADIO FREQUENCY · MHz"), top));
  mount(dial, ui->labDialFreq);
  ui->labDialFreq->setStyleSheet("background: transparent; font-size: 28px; font-weight: bold; border: none;");
  auto preset = new QHBoxLayout;
  mount(preset, ui->bandComboBox);
  mount(preset, ui->readFreq);
  ui->readFreq->setFixedSize(24,24);
  dial->addLayout(preset);
  ui->bandComboBox->setMinimumWidth(205);
  frequencies->addLayout(dial, 0, 0, 2, 1);
  auto rx = new QVBoxLayout;
  rx->addWidget(label(tr("RECEIVE OFFSET · Hz"), top));
  mount(rx, ui->RxFreqSpinBox_2);
  mount(rx, ui->pbR2T_2);
  ui->pbR2T_2->setText(tr("Copy Rx → Tx"));
  frequencies->addLayout(rx, 0, 1, 2, 1);
  auto tx = new QVBoxLayout;
  tx->addWidget(label(tr("TRANSMIT OFFSET · Hz"), top));
  mount(tx, ui->TxFreqSpinBox_2);
  mount(tx, ui->pbT2R_2);
  ui->pbT2R_2->setText(tr("Copy Tx → Rx"));
  frequencies->addLayout(tx, 0, 2, 2, 1);
  auto tolerance = new QVBoxLayout;
  tolerance->addWidget(label(tr("RX TOLERANCE · ±Hz"), top));
  mount(tolerance, ui->sbFtol_2);
  tolerance->addWidget(label(tr("Mode: JTTY"), top));
  frequencies->addLayout(tolerance, 0, 3, 2, 1);
  auto cat = label(QString{}, top);
  cat->setObjectName("jttyCatStatus");
  frequencies->addWidget(cat, 0, 4);
  auto settings = new QPushButton(tr("Radio / Audio…"), top);
  settings->setObjectName("jttySettingsButton");
  connect(settings, &QPushButton::clicked, this, &MainWindow::on_actionSettings_triggered);
  frequencies->addWidget(settings, 1, 4);
  frequencies->addWidget(ui->monitorButton, 0, 5);
  frequencies->addWidget(ui->stopTxButton, 0, 6);
  frequencies->addWidget(ui->tuneButton, 1, 5);
  frequencies->addWidget(ui->logQSOButton, 1, 6);
  for (auto widget : {ui->monitorButton, ui->stopTxButton, ui->tuneButton, ui->logQSOButton}) {
    widget->setMinimumWidth(110); widget->show();
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
  receivedLayout->addWidget(label(tr("All decoded signals in the waterfall range"), received));
  mount(receivedLayout, ui->lh_decodes_headings_label);
  mount(receivedLayout, ui->decodedTextBrowser);
  receivedLayout->setStretch(receivedLayout->count()-1, 1);
  ui->decodedTextBrowser->setMinimumHeight(60);
  receivedLayout->addWidget(label(tr("Click a waterfall signal to select its Rx offset."), received));

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
  contacts->addWidget(label(tr("Call next"), conversation));
  mount(contacts,ui->lineEdit);
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
  if (reading) mount(waterfallHeader,reading);
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
  auto update = [this,cat,scope,devices,counter,reading,meter] {
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
  auto messages=findChild<QSplitter*>("jttyMessagesSplitter");
  auto conversation=findChild<QWidget*>("jttyConversationPanel");
  auto floating=findChild<QDialog*>("jttyFloatingConversation");
  auto popout=findChild<QPushButton*>("jttyPopoutConversation");
  if(!messages || !conversation || !floating || !popout) return false;
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
