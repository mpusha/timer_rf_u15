
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
  setWindowTitle(tr("Program for setup RF timer"));
  //slot_writeSettings();

  modifyData=false;
  dev=new THwBehave;
  connect(dev, SIGNAL(signalMsg(QString,int)), this, SLOT(slot_ProcessMsg(QString,int)));
  //dev->start(QThread::NormalPriority);
  setMinimumSize(800,320);
  //showMaximized();
  resize(800,320);
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
  write_btn=new QPushButton(tr("Write"),this);

  edit_layout = new QHBoxLayout;

  edit_layout->addWidget(refresh_btn); edit_layout->addStretch(1);
  edit_layout->addWidget(write_btn); edit_layout->addStretch(1);

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
 connect(refresh_btn,SIGNAL(pressed()), this, SLOT(slot_updateHW()));
 connect(write_btn,SIGNAL(pressed()), this, SLOT(slot_writeHW()));

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
  time_Label = new QLabel(this);

  time_Label->setFixedWidth(220);
  time_Label->setAlignment(Qt::AlignLeft);

  QLabel *version_label = new QLabel(this);
  version_label->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  version_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  version_label->setText("Program version: " + QCoreApplication::applicationVersion() + " ");

  status_Label=new QLabel(tr("Start program"),this);
  status_Label->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  hwver_Label=new QLabel(tr("HW version: unknown"),this);
  hwver_Label->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  statusBar()->addWidget(time_Label,1);
  statusBar()->addWidget(status_Label,2);
  statusBar()->addWidget(hwver_Label,1);
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
    time_Label->setText(tim);


    QStringList sl;

    //qDebug()<<"STATUS"<<sl;
    if(!sl.empty()){
      status_Label->setText(" Status: "+sl[2]);
      hwver_Label->setText(" Error: "+sl[1]);
      if(sl[3]=='0') {
        alarmWrTimer->stop();
      }
    }
    timer->start(2000);
}




void TTimerRf::slot_alarmWriteAnswer()
{
  alarmWrTimer->stop();
  timer->stop();
  status_Label->setText(" Status: Data don't writen");
  hwver_Label->setText(" Error: HW error");
  QMessageBox::warning(this,"error",tr("Can't write data into HW"));
  timer->start(2000);
}

void TTimerRf::slot_ProcessMsg(QString msg, int code)
{
  if(code==2){
      QMessageBox::critical(this,"Error",msg);
    //QMessageBox msgBox;

    //msgBox.setIcon(QMessageBox::Critical);
    //msgBox.setText(msg);
    //msgBox.exec();
    qApp->closeAllWindows();
  }
  else if(code==1){
    status_Label->setText(msg);
  }
}
