
#include "main_timerrf.h"
//-----------------------------------------------------------------------------
//--- Constructor
//-----------------------------------------------------------------------------
TTimerRf::TTimerRf(QWidget *parent)  : QMainWindow(parent)
{
  create_ListWidget();
  create_Menu();
  create_StatusBar();

  QApplication::processEvents();

  //... Text Codec ...
  QTextCodec *russianCodec = QTextCodec::codecForName("UTF-8");
  QTextCodec::setCodecForLocale(russianCodec);

  //... Data&Time ...
  timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(slot_updateDateTime()));
  //slot_updateDateTime();
  timer->start(2000);
  alarmWrTimer = new QTimer(this);
  connect(alarmWrTimer, SIGNAL(timeout()), this, SLOT(slot_alarmWriteAnswer()));
  setWindowTitle(tr("Program for setup RF amplitude"));
  //slot_writeSettings();

  modifyData=false;
  dev=new THwBehave;
  dev->start(QThread::NormalPriority);
  setMinimumSize(800,600);
  //showMaximized();
  resize(800,600);
}

//-----------------------------------------------------------------------------
//--- Destructor
//-----------------------------------------------------------------------------
TTimerRf::~TTimerRf()
{    
  for(int i=0;i<8;i++) {
    delete itemTable[0][i];
  }
  delete dev;
}
void TTimerRf::keyPressEvent(QKeyEvent *event)
{
   qDebug()<<"keyPressed"<<event->key();
   modifyData=true;
   QWidget::keyPressEvent(event);
}
//-----------------------------------------------------------------------------
//--- create ListWidget
//-----------------------------------------------------------------------------
void TTimerRf::create_ListWidget()
{
  MainGroupBox = new QGroupBox(tr("Prepare data"),this);
  MainGroupBox->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Ignored);
  setCentralWidget(MainGroupBox);
  main_layout = new QVBoxLayout();
  main_layout->setMargin(5);
  main_layout->setSpacing(4);

  refresh_btn=new QPushButton(tr("Refresh"),this);
  read_btn=new QPushButton(tr("Load"),this);
  save_btn=new QPushButton(tr("Save"),this);
  savecsv_btn=new QPushButton(tr("Execute"),this);
  mult_btn=new QPushButton(tr("Multiply"),this);
  undo_btn=new QPushButton(tr("< Undo"),this);
  undo_btn->setEnabled(false);
 // time1_Label=new QLabel(tr("Time step, ms"),this);
 // transf_Label=new QLabel(tr("Coefficient of transf Vrf/Vgf"),this);

  //coefTransf_Edit=new QSpinBox(this); coefTransf_Edit->setMinimum(1); coefTransf_Edit->setMaximum(10000);

  multCoeffLabel=new QLabel(tr("Multiply coefficien for RF amplitude"));
  multCoeffSB=new QDoubleSpinBox();
  multCoeffSB->setSingleStep(0.01);
  multCoeffSB->setMaximum(2);
  multCoeffSB->setMinimum(0.1);
  multCoeffSB->setDecimals(2);
  multCoeffSB->setValue(1);

  edit_layout = new QHBoxLayout;
  edit_layout->addWidget(read_btn); edit_layout->addStretch(1);
  edit_layout->addWidget(save_btn); edit_layout->addStretch(1);
  edit_layout->addWidget(refresh_btn); edit_layout->addStretch(1);
  edit_layout->addWidget(undo_btn); edit_layout->addStretch(1);
  edit_layout->addWidget(savecsv_btn); edit_layout->addStretch(1);
  edit_layout->addWidget(multCoeffLabel); //edit_layout->addStretch(1);
  edit_layout->addWidget(multCoeffSB); edit_layout->addStretch(1);
  edit_layout->addWidget(mult_btn); edit_layout->addStretch(1);

  //edit_layout->addWidget(transf_Label);edit_layout->addWidget(coefTransf_Edit); edit_layout->addStretch(1);
  edit_layout->setSpacing(10);

  data_layout=new QHBoxLayout;

  tableRf=new QTableWidget(this);
  //tableRf->verticalHeader()->hide();
  tableRf->setRowCount(1);
  tableRf->setColumnCount(ALLVECTORS);
  //QTableWidgetItem *labelItem1= new QTableWidgetItem(tr("time, us"));labelItem1->setBackgroundColor(HEADER_COLOR); tableRf->setItem(0,0,labelItem1);
  QStringList headerV,headerH;
  headerH<<"1"<<"2"<<"3"<<"4"<<"5"<<"6"<<"7"<<"8";
  headerV<<tr("time, ms");
  tableRf->setHorizontalHeaderLabels(headerH);
  tableRf->setVerticalHeaderLabels(headerV);
  tableRf->horizontalHeader()->setStretchLastSection(true);
  //tableRf->verticalHeader()->setStretchLastSection(false);
  //tableRf->setMinimumHeight(50);
  //tableRf->setMaximumHeight(50);
  //for(int i=1;i<=ALLVECTORS;i++)
  //  tableRf->setColumnWidth(i,200);
  //tableRf->resizeColumnsToContents();
  data_layout->addWidget(tableRf);


  main_layout->addLayout(data_layout);

  main_layout->addLayout(edit_layout);

  MainGroupBox->setLayout(main_layout);

// create table items
 for(int i=0;i<ALLVECTORS;i++) {
   itemTable[0][i]=new QTableWidgetItem();
   tableRf->setItem(0,i,itemTable[0][i]);

   tableRf->setColumnWidth(i,80);
 }
 putDataToTable();
 connect(refresh_btn,SIGNAL(pressed()), this, SLOT(slot_plotGraph()));
 connect(save_btn,SIGNAL(pressed()), this, SLOT(slot_saveDataFile()));
 connect(read_btn,SIGNAL(pressed()), this, SLOT(slot_readDataFile()));
 connect(savecsv_btn,SIGNAL(pressed()), this, SLOT(slot_saveCSVDataFile()));
 connect(mult_btn,SIGNAL(pressed()), this, SLOT(slot_multData()));
 connect(undo_btn,SIGNAL(pressed()), this, SLOT(slot_undoData()));
}

void TTimerRf::putDataToTable(void)
{
  for(int i=0;i<ALLVECTORS;i++) {
    data[0][i]=round(data[0][i]*100)/100.0;
    itemTable[0][i]->setText(QString("%1").arg(data[0][i],5,'f',2));
  }
}

void TTimerRf::getDataFromTable(void)
{
  bool ok;
  double tmp;
  for(int i=0;i<ALLVECTORS;i++) {
    tmp=itemTable[0][i]->text().toDouble(&ok);
    if(tmp<0) tmp=0;
    else if(tmp>maxTime)
      tmp=maxTime;
    tmp=round(tmp*100);
    data[0][i]=tmp/100.0;
    if(!ok) data[0][i]=0;
  }
}




//-----------------------------------------------------------------------------
//--- create Menu
//-----------------------------------------------------------------------------
void TTimerRf::create_Menu()
{
  QFont font = app_font;
  font.setBold(false);
  menuBar()->setFont(font);

  file_menu = menuBar()->addMenu(tr("&File"));
  exit = new QAction(tr("Exit"), this);
  file_menu->addAction(exit);
  connect(exit,SIGNAL(triggered(bool)), this, SLOT(close()));

}

//-----------------------------------------------------------------------------
//--- create StatusBar
//-----------------------------------------------------------------------------
void TTimerRf::create_StatusBar()
{
  mode_label = new QLabel(this);

  mode_label->setFixedWidth(240);
  mode_label->setAlignment(Qt::AlignCenter);

  QLabel *version_label = new QLabel(this);
  version_label->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  version_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  version_label->setText("ver. " + QCoreApplication::applicationVersion() + " ");

  workfile_label = new QLabel(this);
  workfile_label->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  workfile_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  status_Label=new QLabel(tr("Start program"),this);
  status_Label->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  err_Label=new QLabel(tr("unknown"),this);
  err_Label->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  statusBar()->addWidget(mode_label);
  statusBar()->addWidget(workfile_label,1);

  statusBar()->addWidget(status_Label,1);
  statusBar()->addWidget(err_Label,1);
  statusBar()->addWidget(version_label,1);

  QFont font = app_font;
  font.setBold(false);
  statusBar()->setFont(font);
}

//-----------------------------------------------------------------------------
//--- Sleep
//-----------------------------------------------------------------------------
void TTimerRf::Sleep(int ms)
{
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, SLOT(quit()));
    loop.exec();
}

//------------------------------ SLOTS ----------------------------------------
//-----------------------------------------------------------------------------
//--- Slot update_DateTime()
//-----------------------------------------------------------------------------
void TTimerRf::slot_updateDateTime()
{
    timer->stop();
    QString tim = QDateTime::currentDateTime().toString(" d MMMM dddd yyyy, h:mm:ss ");
    mode_label->setText(tim);

    workfile_label->setText("Work file:  ");
    QStringList sl;

    //qDebug()<<"STATUS"<<sl;
    if(!sl.empty()){
      status_Label->setText(" Status: "+sl[2]);
      err_Label->setText(" Error: "+sl[1]);
      if(sl[3]=='0') {
        alarmWrTimer->stop();
        savecsv_btn->setEnabled(true);
      }
    }
    timer->start(2000);
}




void TTimerRf::slot_alarmWriteAnswer()
{
  alarmWrTimer->stop();
  timer->stop();
  status_Label->setText(" Status: Data don't writen");
  err_Label->setText(" Error: HW error");
  QMessageBox::warning(this,"error",tr("Can't write data into HW"));
  savecsv_btn->setEnabled(true);
  timer->start(2000);
}

