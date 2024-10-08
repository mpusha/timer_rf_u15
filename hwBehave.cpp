/*!
*  \file hwBehave.cpp
*  \brief Файл с реализацией класса THwBehave.
*/
#include <QThread>
#include <QDir>
//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-1
//https://habr.com/ru/articles/252101/
#include "hwBehave.h"

/*!
 * @brief Конструктор класса THwBehave.
 *
 * Создает объект внутреннего таймера для опроса внешнего устройства.
 * Считывает уставки переменных и запускает поток.
 */
THwBehave::THwBehave()
{

  qDebug()<<"Start constructor of object THwBehave";

  tAlrm=new QTimer();
  readSettings();
  serial=0;
  pastSt=0; presentSt=0;
  connect(tAlrm,SIGNAL(timeout()),this,SLOT(slotTimeAlarm()));
  connect(this, SIGNAL(signalTimerEnable(bool)), this, SLOT(slotTimerEnable(bool)));
  start(QThread::NormalPriority);

  qDebug()<<"End constructor of object THwBehave";
}

/*!
 * @brief Деструктор класса THwBehave.
 *
 * Останавливает внутренний таймер и прерывает выполнение потока.
 * Удаляет последовательное устройство.
 */
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

/*!
 * @brief qDebug operator owerwrite for print states in debug mode
 */
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
  default:  dbg.space()                    << "UNKNOWN_STATE" ; break;
  }
  return dbg.nospace() ;//<< endl;;
}

/*!
 * \brief Запуск основного цикла обработчика потока.
 *
 * Создание экземпляра QSerialPort и запуск потока, в котором
 * работает обрабочик автомата состояний. Автомат настраивается на
 * состояние #INITIAL_STATE по обработке которого происходит инициализация
 * последовательного порта. В случае ошибки поток прерывается и в пользовательский
 * интерфейс отправляется сообщение об неуспешной инициализации устройства.
 * внутри потока управление работой внутреннего таймера осуществляется с
 * использованием сигнала signalTimerEnable().
*/
void THwBehave::run()
{
  qDebug()<<"Start run() cycle of object THwBehave";
  abort=false;
  serial=new QSerialPort();
  phase=INITIAL_STATE;
  for(int i=0;i<ALLREQSTATES;i++) allStates[i]=READY;
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
        hwError=decodeErrorStr(hwErr);
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
            hwError=decodeErrorStr(hwErr);
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
        hwError=decodeErrorStr(hwErr);
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

//------------------------------------------Public methods
/*!
 * @brief Декодирует код ошибки в строку.
 *
 * Возвращает строку с описанием ошибки.
 * \param [in] st код ошибки.
 * \return строка с кодом ошибки.
 */
QString THwBehave::decodeErrorStr(int st)
{
  return(cErrStr[abs(st)]);
}

/*!
 * @brief Чтение настроек.
 *
 * Чтение пользовательских настроек из файла setup.ini, находящегося
 * в одном каталоге с исполняемой программой. Считываются имя устройства последовательного порта,
 * скорости и адреса таймера.
 */
void THwBehave::readSettings(void)
{
  QString dir_path = qApp->applicationDirPath();
  QSettings setup(dir_path+"/setup.ini", QSettings::IniFormat);
  bool ok;
  serialPortName=setup.value("port","/dev/ttyUSB0").toString();
  serialSpeed=setup.value("speed",9600).toInt(&ok); if(!ok) serialSpeed=QSerialPort::Baud9600;
  address=setup.value("address",1).toInt(&ok); if(!ok) address=1;
}

/*!
 * @brief Чтение установленных значений времени на каналах таймера.
 *
 * Чтение в переменную time[] значений временных уставок по каналам в мкс.
 * В случае ошибки устанавливается отрицательное значение и возвращается код ошибки.
 * Если ошибки нет возвращается #ERR_NONE.
 * \return код ошибки.
 */
int THwBehave::readTime(void)
{
  int ti[ALLVECTORS],err;
  for(int i=0;i<ALLVECTORS;i++) time[i]=-100;
  for(int i=0;i<ALLVECTORS;i++){
    err=readData("RT",i+1,&ti[i]);
    if(err) return err;
  }
  for(int i=0;i<ALLVECTORS;i++) time[i]=ti[i];
  return 0;
}

/*!
 * @brief Инициализация устройства последовательного порта.
 *
 * Инициализация последовательного порта. Возвращается код ошибки.
 * Если ошибки нет возвращается #ERR_NONE.
 * \return код ошибки.
 */
int THwBehave::initialDevice(void)
{
  serial->setPortName(serialPortName);
  serial->setBaudRate(serialSpeed);

 if (!serial->open(QIODevice::ReadWrite)) {
   return 1;
 }
  return 0;
}

/*!
* @brief Установка автомата состояния.
*
* Переход в новое состояние. Переход происходит во время не заблокированного мьютекса, т.е.
* когда переменная состояния не используется для управления.
* \param [in] state новое сосотояние аппарата. Возможны следующие состояния:
* #GETSTATUS_STATE, #WRITE_STATE, #READ_STATE, #UPDATE_STATE
*/
void THwBehave::setState(CPhase state)
{
  mutex.lock(); allStates[state]=state; mutex.unlock(); condition.wakeOne();
}

/*!
* @brief Выход из цикла обработки run().
*
* Выход из цикла обработчика потока.
* \param [in] ab - если установлена в true то осуществляется выход.
*/
void THwBehave::setAbort(bool ab)
{
  abort=ab; condition.wakeOne();
}

/*!
* @brief Чтение установленного времени срабатывания канала таймера.
*
* Чтение установленного времени срабатывания канала таймера в мкс.
* \param [in] index - номер канала.
* \return время в мкс.
*/
int THwBehave::getTime(int index)
{
  return time[index];
}

/*!
* @brief Запись установленного времени срабатывания канала таймера.
*
* Запись установленного времени срабатывания канала таймера в мкс.
* \param [in] index - номер канала.
* \param [in] val - время в мкс.
*/
void THwBehave::setTime(int index,int val)
{
  time[index]=val;
}

//--------------------------------------Private methods
/*!
* @brief Выполнение команды в таймере.
*
* Запись по последовательному каналу в микроконтроллер с адресом address таймера команды, и прием ответа.
* Команда может быть AL,SM,UH.
* Если ошибки нет возвращается #ERR_NONE.
* \param [in] command - строка с командой.
* \return код ошибки.
*/
int THwBehave::execCmd(QString command)
{
  int codret=0;

  QString cmd=QString("%1:%2\0").arg(address,2,10,QChar('0')).arg(command);
  QString answer;

  serial->write(cmd.toLocal8Bit().data(),cmd.size()+1);
  if (!serial->waitForBytesWritten(SERIAL_TOUT)) return ERR_UART_TRANS;
  // read response
  msleep(RS_DELAY);
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
  return codret;
}

/*!
* @brief Отправка команды в таймер.
*
* Запись по последовательному каналу в микроконтроллер таймера команды.
* Если ошибки нет возвращается #ERR_NONE.
* \param [in] cmd - строка с командой.
* \return код ошибки.
*/
int THwBehave::sendCmd(QString cmd)
{
  serial->write(cmd.toLocal8Bit().data(),cmd.size()+1);
  if (!serial->waitForBytesWritten(SERIAL_TOUT)) return ERR_UART_TRANS;
  return ERR_NONE;
}

/*!
* @brief Получение ответа на переданную команду.
*
* Ожидание и прием ответа на заранее переданную команду.
* Если ошибки нет возвращается #ERR_NONE.
* \param [in] answer - строка с командой.
* \return код ошибки.
*/
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

/*!
* @brief Чтение из таймера строки символов.
*
* Передача команды, по выполнению которой получаем строку символов.
* Это может быть статус устройства.
* Команда может быть RS, GF.
* Если ошибки нет возвращается #ERR_NONE.
* \param [in] cmd - строка с командой.
* \param [in] ans - прочитанная строка.
* \return код ошибки.
*/
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

/*!
* @brief Чтение из таймера данных о времени срабатывания канала.
*
* Чтение из таймера данных о времени срабатывания канала в мкс.
* Команда может быть RT, GT.
* Если ошибки нет возвращается #ERR_NONE.
* \param [in] cmd - строка с командой.
* \param [in] ch - номер канала от 1 до 8
* \param [in] *readData - указатель на прочитанные из таймера данные в мкс.
* \return код ошибки.
*/
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

/*!
* @brief Запись в канал таймера данных о времени срабатывания.
*
* Запись в таймер данных о времени срабатывания канала в мкс.
* Команда может быть ST, WT.
* Если ошибки нет возвращается #ERR_NONE.
* \param [in] cmd - строка с командой.
* \param [in] ch - номер канала от 1 до 8
* \param [in] *data - время в мкс.
* \return код ошибки.
*/
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

//----------------------------------------- public slots
/*!
 * @brief Вызов для разрешении/запрещении внутреннего таймера.
 *
 * Используется для запуска/остановки внутреннего таймера из другого потока.
 * \param [in] en - true разрешает работу таймера
 */
void THwBehave::slotTimerEnable(bool en)
{
  if(en) tAlrm->start(SAMPLE_DEVICE); else tAlrm->stop();
}

/*!
 * @brief Вызов по окончанию отсчета интервала внутреннего таймера.
 *
 * Таймаут, возникающий по срабатыванию внутреннего таймера.
 * Используется для опроса внешнего устройства.
 */
void THwBehave::slotTimeAlarm(void)
{
  //qDebug()<<"TIMER'";
  allStates[GETSTATUS_STATE]=GETSTATUS_STATE;
  condition.wakeOne();
}
