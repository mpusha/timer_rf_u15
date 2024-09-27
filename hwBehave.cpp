#include <QThread>
#include <QDir>

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

  connect(tAlrm,SIGNAL(timeout()),this,SLOT(timeAlarm()));
  tAlrm->start(SAMPLE_DEVICE); //start timer for sample device
  start(QThread::NormalPriority);
  connect(this, SIGNAL(signalTimerEnable(bool)), this, SLOT(slotTimerEnable(bool)));
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

  qDebug()<<"End destructor of object THwBehave";
}


// qDebug operator owerwrite for print states in debug mode
QDebug operator <<(QDebug dbg, const CPhase &t)
{
  dbg.nospace() <<"STATE="<<(int) t;
  switch(t){
  case READY: dbg.space()                  << "READY" ; break;
  case IDLE: dbg.space()                   << "IDLE" ; break;
  case GETSTATUS_STATE: dbg.space()        << "GETSTATUS_STATE" ; break;
  case INITIAL_STATE: dbg.space()          << "INITIAL_STATE" ; break;
  case GLOBAL_ERROR_STATE: dbg.space()     << "GLOBAL_ERROR_STATE" ; break;
  case DEVICE_ERROR_STATE: dbg.space()     << "DEVICE_ERROR_STATE"; break;
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
void THwBehave::setErrorSt(short int st)
{

}

//-----------------------------------------------------------------------------
//--- timer timeout event. On this event can get data from device
//-----------------------------------------------------------------------------
void THwBehave::timeAlarm(void)
{
  qDebug()<<"Timer";

  allStates[GETSTATUS_STATE]=GETSTATUS_STATE;
  condition.wakeOne();
}

//-----------------------------------------------------------------------------
//--- Run process. Main cycle with state machine
//-----------------------------------------------------------------------------
void THwBehave::run()
{
  abort=false;
  phase=INITIAL_STATE;
  for(int i=0;i<ALLREQSTATES;i++) allStates[i]=READY;
  emit signalTimerEnable(true);
  qDebug()<<"Start run() cycle of object THwBehave";
  int repInit=3;
  CPhase deb=READY;//for debug only
  //QEventLoop loop;
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

// Sample device request from timer
      case GETSTATUS_STATE: {
        //getInfoData();
        if(reInit) { phase=GETINFO_STATE; break; }
        allStates[GETSTATUS_STATE]=READY; // reset state
        phase = READY;
        break;
      }//end case GETPARREQ_STATE:

// Found global error. Server can't work property. Wait until restart INITIAL_STATE
      case GLOBAL_ERROR_STATE: {
        abort=true;
        msleep(200);
        break;
      }//end case GLOBAL_ERROR_STATE

// connect to device and get simple information
      case INITIAL_STATE: {
        int err=initialDevice();
        if(err) {
          emit signalMsg("Can't find serial device. Reboot computer.",2);
          phase=GLOBAL_ERROR_STATE;
        }
        else {
          phase = READY;
        }
        break;
      }// end case INITIAL_STATE:

//  get info about device
      case GETINFO_STATE: {

        int err=getInfoDevice();
        //setErrorSt(::CODEERR::NONE);
       // timerAlrm=true;

        if(err){

          phase=DEVICE_ERROR_STATE;
        }
        else {

          repInit=1;
          phase = READY;
        }
        break;
      }//end case GETINFO_STATE:

// processing hardware errors in device (CAN, bulk)
      case DEVICE_ERROR_STATE:{

        if(repInit==3) {
          phase = INITIAL_STATE;
          repInit=2;
        }
        else if(repInit==2) {
          phase = GLOBAL_ERROR_STATE;
        }
        else
          phase = READY;
       // nextPhase=READY;
        break;
      }// end case DEVICE_ERROR_STATE:
    } // End Switch main state machine--------------------------------------------------------------------------------------------------------------
    if(abort) break;
  }
  emit signalTimerEnable(false);
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
  serialPort=setup.value("port","ttyS0").toString();
  serialSpeed=setup.value("speed",9600).toInt(&ok); if(!ok)serialSpeed=QSerialPort::Baud9600;
}

//================= DEVICE PART ================================================================================================================
//-----------------------------------------------------------------------------
//--- Initialise device INITIAL_STATE in state machine
//-----------------------------------------------------------------------------
int THwBehave::initialDevice(void)
{
  return 1;
}

//-------------------------------------------------------------------------------------------------
//--- Get information about device
//-------------------------------------------------------------------------------------------------
int THwBehave::getInfoDevice()
{

  return 0;
}

// private slots
void THwBehave::slotTimerEnable(bool en)
{
  if(en) tAlrm->start(SAMPLE_DEVICE); else tAlrm->stop();
}
