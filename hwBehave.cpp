#include <QThread>
#include <QDir>
//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-1
#include "hwBehave.h"

/**
 * @brief TDtBehave::TDtBehave
 * @param nPort
 * @param dName
 * @param usbMtx
 * @param vid
 * @param pid
 * @param logSize
 * @param logCount
 */
THwBehave::THwBehave()
{

  qDebug()<<"Start constructor of object THwBehave";

  tAlrm=new QTimer();
  reInit=true; // reinit timer (get version, read data)
  readSettings();
  serial=0;
  pastSt=0; presentSt=0;
  connect(tAlrm,SIGNAL(timeout()),this,SLOT(timeAlarm()));
  connect(this, SIGNAL(signalTimerEnable(bool)), this, SLOT(slotTimerEnable(bool)));
  start(QThread::NormalPriority);

  qDebug()<<"End constructor of object THwBehave";
}

//-----------------------------------------------------------------------------
//--- Destructor
//-----------------------------------------------------------------------------
THwBehave::~THwBehave()
{
  qDebug()<<"Start destructor of object THwBehave";
  tAlrm->deleteLater();
  if(isRunning()){
    setAbort(true);
    condition.wakeOne();
    mutex.unlock();
  }
  wait(1000);
  if(isRunning()){
    qDebug()<<"Object THwBehave still running... terminate() calling";
    terminate();
    wait(1000);
  }
  if(serial) delete serial;
  qDebug()<<"End destructor of object THwBehave";
}


// qDebug operator owerwrite for print states in debug mode
QDebug operator <<(QDebug dbg, const CPhase &t)
{
  dbg.nospace() <<"STATE=";
  switch(t){
  case READY: dbg.space()                  << "READY" ; break;
  case IDLE: dbg.space()                   << "IDLE" ; break;
  case WRITE_STATE: dbg.space()            << "WRITE_STATE" ; break;
  case UPDATE_STATE: dbg.space()           << "UPDATE HW INFO" ; break;
  case GETSTATUS_STATE: dbg.space()        << "GETSTATUS_STATE" ; break;
  case INITIAL_STATE: dbg.space()          << "INITIAL_STATE" ; break;
  case GLOBAL_ERROR_STATE: dbg.space()     << "GLOBAL_ERROR_STATE" ; break;
  case SEND_STATE: dbg.space()             << "SEND_STATE"; break;
  case GETINFO_STATE: dbg.space()          << "GETINFO_STATE" ; break;
  case TIMER_START_STATE: dbg.space()      << "TIMER_START_STATE"; break;
  case TIMER_STOP_STATE: dbg.space()       << "TIMER_STOP_STATE"; break;
  default:  dbg.space()                    << "UNKNOWN_STATE" ; break;
  }
  return dbg.nospace() ;//<< endl;;
}
//-----------------------------------------------------------------------------
//--- State all error Status for all stadies
//-----------------------------------------------------------------------------
QString THwBehave::getErrorStr(int st)
{
  return(cErrStr[abs(st)]);
}

//-----------------------------------------------------------------------------
//--- timer timeout event. On this event can get data from device
//-----------------------------------------------------------------------------
void THwBehave::timeAlarm(void)
{
  //qDebug()<<"TIMER'";
  allStates[GETSTATUS_STATE]=GETSTATUS_STATE;
  condition.wakeOne();
}

//-----------------------------------------------------------------------------
//--- Run process. Main cycle with state machine
//-----------------------------------------------------------------------------
void THwBehave::run()
{
  abort=false;
  serial=new QSerialPort();
  phase=INITIAL_STATE;
  for(int i=0;i<ALLREQSTATES;i++) allStates[i]=READY;
  qDebug()<<"Start run() cycle of object THwBehave";

  CPhase deb=READY;//for debug only
  while(!abort) { // run until destructor not set abort
    mutex.lock();
    if(phase==READY){
      for(int i=0;i<ALLREQSTATES;i++) { // read all statese request and run if state!= READY state. high priority has low index
        if(allStates[i]!=READY){
          phase=allStates[i];
          allStates[i]=READY;
          break;
        }
      }
    }
    if(phase==READY){
      phase=IDLE;
      condition.wait((&mutex));
      phase=READY;
    }
    mutex.unlock();

    if(deb!=phase) { qDebug()<<phase;deb=phase;}
    switch(phase) {
      default: {
        for(int i=0;i<ALLREQSTATES;i++) allStates[i]=READY;
        phase = INITIAL_STATE;
        break;
      }
      case READY:{
        break;
      }
// connect to serial port
      case INITIAL_STATE: {
        int err=initialDevice();
        if(err) {
          emit signalMsg("Can't find serial device "+serialPortName+" Reboot computer.",3);
          phase=GLOBAL_ERROR_STATE;
        }
        else {
          phase = GETSTATUS_STATE;
          emit signalTimerEnable(true);
        }
        break;
      }// end case INITIAL_STATE:
//  get info about device
      case GETINFO_STATE: {
        int hwErr;
        hwErr=execCmd("AL"); //test alive
        if(!hwErr) { // errors absent
          hwErr=readStr("GF",hwVersion);
          hwErr=readStr("RS",hwStatus);
          hwErr=readTime();
          emit signalDataReady(0);
          emit signalMsg("",5); // device present
        }
        else {
          hwStatus="Can't find timer";
          hwVersion="unknown";
          emit signalMsg("",4); // device absent
        }
        hwError=getErrorStr(hwErr);
        phase = SEND_STATE;
        break;
      }//end case GETINFO_STATE:
// Found global error. Server can't work properly.
      case GLOBAL_ERROR_STATE: {
        abort=true;
        break;
      }//end case GLOBAL_ERROR_STATE
// message about state of device
      case SEND_STATE:{
        emit signalMsg(hwError,2);
        emit signalMsg(hwStatus,1);
        emit signalMsg(hwVersion,0);
        phase = READY;
        break;
      }// end case DEVICE_ERROR_STATE:
// find arrive leive
      case GETSTATUS_STATE: {
         int hwErr;
         hwErr=execCmd("AL"); //test alive
         if(!hwErr) { // errors absent
           presentSt=1;
         }
         else
           presentSt=0;
         if((presentSt ^ pastSt)&presentSt){ // arrive device
           phase = GETINFO_STATE;
         }
         else if((presentSt ^ pastSt)&pastSt){ // leive device
           phase = GETINFO_STATE;
         }
         else {
           phase = READY;
         }
         pastSt=presentSt;
         allStates[GETSTATUS_STATE]=READY; // reset state
         break;
      }//end case GETPARREQ_STATE
      case UPDATE_STATE: {
        allStates[UPDATE_STATE]=READY;
        phase=GETINFO_STATE;
        break;
      }//end case UPDATE_STATE
      case WRITE_STATE: {
        QString ans;
        int hwErr;
        allStates[WRITE_STATE]=READY;
        hwErr=execCmd("AL"); //test alive
        if(!hwErr) { // errors absent
          for(int i=0;i<ALLVECTORS;i++) {
            hwErr=writeData("ST",i+1,time[i]); //write time data
            if(hwErr) break;
          }
          if(hwErr){
            hwError=getErrorStr(hwErr);
            hwStatus="data don't write";
            phase = SEND_STATE;
            break;
          }
          hwErr=execCmd("UH");
          if(!hwErr){
            do {
              msleep(900);
              ans.clear();
              if(!readStr("RS",ans)) hwStatus=ans;// read status
              else
                hwStatus="unknown";
              emit signalMsg(hwStatus,1);
            } while(execCmd("SM")); // read state machine
          }
        }
        else {
          phase=GETINFO_STATE;
          break;
        }
        emit signalMsg("",5); // device present
        hwError=getErrorStr(hwErr);
        phase = SEND_STATE;
        //phase=READY;
        break;
      }  //end case WRITE_STATE
    } // End Switch main state machine--------------------------------------------------------------------------------------------------------------
    if(abort) break;
  }
  emit signalTimerEnable(false);
  serial->close();
  qDebug()<<"End run() cycle of object THwBehave";
}

//-----------------------------------------------------------------------------
//--- public readSettings()
//-----------------------------------------------------------------------------
void THwBehave::readSettings(void)
{
  QString dir_path = qApp->applicationDirPath();
  QSettings setup(dir_path+"/setup.ini", QSettings::IniFormat);
  bool ok;
  serialPortName=setup.value("port","/dev/ttyUSB0").toString();
  serialSpeed=setup.value("speed",9600).toInt(&ok); if(!ok) serialSpeed=QSerialPort::Baud9600;
  address=setup.value("address",1).toInt(&ok); if(!ok) address=1;
}

//================= DEVICE PART ================================================================================================================
//-----------------------------------------------------------------------------
//--- Initialise device INITIAL_STATE in state machine
//-----------------------------------------------------------------------------
int THwBehave::initialDevice(void)
{
  serial->setPortName(serialPortName);
  serial->setBaudRate(serialSpeed);

 if (!serial->open(QIODevice::ReadWrite)) {
   return 1;
 }
  return 0;
}

int THwBehave::readTime()
{
  int ti[ALLVECTORS],err;
  for(int i=0;i<ALLVECTORS;i++) time[i]=-100;
  for(int i=0;i<ALLVECTORS;i++){
    err=readData("GT",i+1,&ti[i]);
    if(err) return err;
  }
  for(int i=0;i<ALLVECTORS;i++) time[i]=ti[i];
  return 0;
}

// private slots
void THwBehave::slotTimerEnable(bool en)
{
  if(en) tAlrm->start(SAMPLE_DEVICE); else tAlrm->stop();
}

// return 0 if controller alive
int THwBehave::execCmd(QString command)
{
  int codret=0;

  QString cmd=QString("%1:%2\0").arg(address,2,10,QChar('0')).arg(command);
  QString answer;

  serial->write(cmd.toLocal8Bit().data(),cmd.size()+1);
  if (!serial->waitForBytesWritten(SERIAL_TOUT)) return ERR_UART_TRANS;
  // read response
  answer.clear();
  QByteArray tmp;
  while(serial->waitForReadyRead(SERIAL_TOUT)){
    tmp=serial->readAll();
    answer.append(tmp);
    if(!tmp.at(tmp.size()-1)) break;
  }
  if(answer.isEmpty()) return ERR_UART_TOUT;
  QStringList rdata;
  bool ok;
  rdata.clear();
  rdata=answer.simplified().split(':');
  if(rdata.count()<2) return ERR_IDATA_CNT;  // no all data
  int addr_c=rdata.at(0).toInt(&ok);
  if(!ok) return ERR_IDATA_ADDR;
  if(addr_c!=address) return ERR_IDATA_ADDR;  // address none correct
  codret=rdata.at(1).toInt(&ok);
  if(!ok) return ERR_IDATA_DATA;
  if(codret) return ERR_BAD;
  return ERR_NONE;
}

int THwBehave::sendCmd(QString cmd)
{
  serial->write(cmd.toLocal8Bit().data(),cmd.size()+1);
  if (!serial->waitForBytesWritten(SERIAL_TOUT)) return ERR_UART_TRANS;
  return ERR_NONE;
}
int THwBehave::readAnswer(QString& answer)
{
  answer.clear();
  QByteArray tmp;
  while(serial->waitForReadyRead(SERIAL_TOUT)){
    tmp=serial->readAll();
    answer.append(tmp);
    if(!tmp.at(tmp.size()-1)) break;
  }
  if(answer.isEmpty()) return ERR_UART_TOUT;
  return ERR_NONE;
}

int THwBehave::readStr(QString cmd,QString& ans)
{
  QString sendStr;
  int ret=0;
  QString answer;
  QStringList rdata;
  bool ok;
  ans="unknown";
  sendStr=QString("%1:%2").arg(address,2,10,QChar('0')).arg(cmd);
  for(int i=0;i<REP;i++){
    msleep(RS_DELAY);
    ret=sendCmd(sendStr);
    if(ret!=ERR_NONE) { serial->flush(); continue; }
    ret=readAnswer(answer);
    if(ret!=ERR_NONE) { serial->flush(); continue; }
    rdata.clear();
    rdata=answer.simplified().split(':');
    if(rdata.count()<2) {ret=ERR_IDATA_CNT; continue; } // no all data
    int addr=rdata.at(0).toInt(&ok);
    if(!ok) {ret=ERR_IDATA_ADDR; continue; }
    if(addr!=address) {ret=ERR_IDATA_ADDR; continue; } // address none correct
    ans=rdata.at(1) ;

    break;
  }
  return ret;
}
// read timer channel setup value
int THwBehave::readData(QString cmd,int ch,int *readData)
{
  QString sendStr;
  int ret=0;
  QString answer;
  QStringList rdata;
  bool ok;

  sendStr=QString("%1:%2 %3").arg(address,2,10,QChar('0')).arg(cmd).arg(ch);
  for(int i=0;i<REP;i++){
    msleep(RS_DELAY);
    ret=sendCmd(sendStr);
    if(ret!=ERR_NONE) { serial->flush(); continue; }
    ret=readAnswer(answer);
    if(ret!=ERR_NONE) { serial->flush(); continue; }
    rdata.clear();
    rdata=answer.simplified().split(':');
    if(rdata.count()<2) {ret=ERR_IDATA_CNT; continue; } // no all data
    int addr=rdata.at(0).toInt(&ok);
    if(!ok) {ret=ERR_IDATA_ADDR; continue; }
    if(addr!=address) {ret=ERR_IDATA_ADDR; continue; } // address none correct
    int codret=rdata.at(1).toInt(&ok);
    if(!ok) {ret=ERR_IDATA_CRET; continue; }
    *readData=codret;
    //if(codret) { ret=codret; continue; } //uc  error
    break;
  }
  return ret;
}

int THwBehave::writeData(QString cmd,int ch, int data)
{

  QString sendStr;
  int ret=0;
  sendStr=QString("%1:%2 %3 %4").arg(address,2,10,QChar('0')).arg(cmd).arg(ch).arg(data);

  QString answer;
  QStringList rdata;
  bool ok;
  for(int i=0;i<REP;i++){
    msleep(RS_DELAY);
    ret=sendCmd(sendStr);
    if(ret!=ERR_NONE) { serial->flush(); continue; }
    ret=readAnswer(answer);
    if(ret!=ERR_NONE) { serial->flush(); continue; }
    rdata.clear();
    rdata=answer.split(':');
    if(rdata.count()<2) {ret=ERR_IDATA_CNT; continue; } // no all data
    int addr=rdata.at(0).toInt(&ok);
    if(!ok) {ret=ERR_IDATA_ADDR; continue; }
    if(addr!=address) {ret=ERR_IDATA_ADDR; continue; } // address none correct
    if(rdata.at(1)[0]=='?') { ret=ERR_BADANSW; continue; } // receive ? in answer
    int codret=rdata.at(1).toInt(&ok);
    if(!ok) {ret=ERR_IDATA_CRET; continue; }
    if(codret) { ret=codret; continue; } //uc  error;
    break;
  }
  return ret;
}
