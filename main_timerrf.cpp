
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

  setWindowTitle(tr("Program for setup RF timer"));

  modifyData=false;
  dev=new THwBehave;
  connect(dev, SIGNAL(signalMsg(QString,int)), this, SLOT(slot_processMsg(QString,int)));
  connect(dev, SIGNAL(signalDataReady(int)), this, SLOT(slot_processData(int)));
  setMinimumSize(800,320);
  //showMaximized();
  resize(900,320);
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

  update_btn=new QPushButton(tr("Update"),this);
  write_btn=new QPushButton(tr("Write"),this);

  edit_layout = new QHBoxLayout;

  edit_layout->addWidget(update_btn); edit_layout->addStretch(1);
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
 connect(update_btn,SIGNAL(pressed()), this, SLOT(slot_updateHW()));
 connect(write_btn,SIGNAL(pressed()), this, SLOT(slot_writeData()));
 tableRf->setEnabled(false);
 write_btn->setEnabled(false);
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
  err_Label = new QLabel("Error: unknown");

  //err_Label->setFixedWidth(150);
  err_Label->setAlignment(Qt::AlignLeft);

  QLabel *version_label = new QLabel(this);
  version_label->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  version_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  version_label->setText("Program version: " + QCoreApplication::applicationVersion() + " ");

  status_Label=new QLabel(tr("Status: Start program"),this);
  status_Label->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  hwver_Label=new QLabel(tr("HW version: unknown"),this);
  hwver_Label->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  statusBar()->addWidget(err_Label,2);
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


void TTimerRf::slot_processMsg(QString msg, int code)
{
  if(code==5){ // device present
    tableRf->setEnabled(true);
    write_btn->setEnabled(true);
  }
  else if(code==4){ // device absent
    tableRf->setEnabled(false);
    write_btn->setEnabled(false);
  }
  else if(code==3) {
    status_Label->setText(msg);
    QTimer::singleShot(10000, qApp, SLOT(closeAllWindows()));
    QMessageBox::critical(this,"Error",msg);
      //QEventLoop loop;

      //loop.exec();
    //QMessageBox msgBox;

    //msgBox.setIcon(QMessageBox::Critical);
    //msgBox.setText(msg);
    //msgBox.exec();
    qApp->closeAllWindows();
  }
  else if(code==2){
    err_Label->setText("Error: "+msg);
  }
  else if(code==1){
    status_Label->setText("Status: "+msg);
  }
  else if(code==0) {
    hwver_Label->setText("HW version: "+msg);
  }
}
void TTimerRf::slot_processData(int code)
{
  if(code==0){
    for(int i=0;i<ALLVECTORS;i++){
      data[0][i]=dev->getTime(i)/1000.0;
    }
    putDataToTable();
  }
}
void TTimerRf::slot_updateHW(void)
{
   dev->setState(UPDATE_STATE);
}

void TTimerRf::slot_writeData(void)
{
  if(modifyData){
    slot_processMsg("",4);// disable btn
    getDataFromTable();
    putDataToTable();
    for(int i=0;i<ALLVECTORS;i++){
      dev->setTime(i,round(data[0][i]*1000));
    }
    dev->setState(WRITE_STATE);
    modifyData=false;
  }
}
