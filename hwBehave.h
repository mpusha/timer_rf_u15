#ifndef HWBEHAVE_H
#define HWBEHAVE_H
/*!
*  \file hwBehave.h
*  \brief Header of class TDtBehave
*/
//#include <QtCore/QCoreApplication>
#include <QtCore>
#include <QFile>
#include <QDebug>
#include <QString>
#include <QTime>
#include <QTimer>
#include <QStringList>
#include <QThread>
#include <QMutex>
#include <QDebug>
#include <QWaitCondition>
#include <QSerialPort>
#include <QDataStream>
#include <QByteArray>

#define ALLVECTORS 8 ///< size
#define SAMPLE_DEVICE 5000 // time in ms for request device data (timer)

//#define STARTING_STS "starting"
//#define BUSY_STS "busy"
//#define READY_STS "ready"
//#define ERR_STS "error"


#define REP 3 // number of repeates

#define ERR_NONE 0
// from 0 to 7 reserv for uc
// 0 - отсутствие ошибки
// 1 - переполнение входного буфера (возможно посылаемвая строка не заканяивается кодом 0)
// 2 - формат входных данных не соответствует ожидаемому scanf
// 3 - ошибка во входных данных. Возможен выход из диапазона или отрицательный знак
// 4 - ошибка при записи в HW (считались данные не соответствующие записываемым)
#define ERR_UART_ABS 8     // отсутствует порт UART. Ошибка инициализации устройства
#define ERR_UART_TRANS 9   // ошибка отправки сообщения
#define ERR_UART_TOUT  10  // при приеме произоше таймаут UART_TIMEOUT
#define ERR_IDATA_CNT 11   // принято колличество данных менее ожидаемого
#define ERR_IDATA_ADDR 12  // приятый адрес не является числом
#define ERR_IDATA_CRET 13  // принятые данные не являются числом
#define ERR_IDATA_DATA 14  // принятые данные не совпадают с записываемыми (при операции проверки)
#define ERR_SETUP_ADDR 15  // адрес устройства не установлен, или выходит из диапазона 0-99
#define ERR_SETUP_CH 16    // устанавливаемый канал таймера выходит из диапазона 1-16, либо не установлен
#define ERR_SETUP_DATA 17  // устанавливаоемое время запуска или длительность выходного имульса выходит из диапазона MINDATA-MAXDATA MINWIDTH-MAXWIDTH
#define ERR_BAD        18  //
#define ERR_BADANSW    19

#define SERIAL_TOUT 100
#define UART_SHORT_TOUT 50L
#define RS_DELAY 5
/**
*/
typedef enum
{
  GETSTATUS_STATE, ///< no errors
  WRITE_STATE,
  READ_STATE,
  UPDATE_STATE,
  TIMER_START_STATE,
  TIMER_STOP_STATE,
  ALLREQSTATES,    ///<limiter on process request states
    IDLE,
    READY,
    INITIAL_STATE,
    GETINFO_STATE,
    GLOBAL_ERROR_STATE,
    SEND_STATE
}CPhase;  // phases of state machine

const QString cErrStr[]={"none","input buffer overflow","scanf format","range input data is incorrect","HW write", //4
                         "none","none","none", //7
                         "UART port absent","can't send message","receiver timeout","count of input data is incorrect",//11
                         "address is incorrect", "time is incorrect","write/read data are different", //14
                         "address of device don't set","setup chanel of timer is outrange","setup time of timer is outrange","timer off",//18
                         "bad answer from uC"}; //19

/*! \brief Class THwBehave
 *  \date Oct 2024
 * \author Sergey Sytov
*/
class THwBehave : public QThread
{
  Q_OBJECT
public:
  THwBehave();
  ~THwBehave();
  void readSettings(void);
  void setState(CPhase state) { mutex.lock(); allStates[state]=state; mutex.unlock(); condition.wakeOne(); }

// global error function


// working with HW
  QString getErrorStr(int); // get error string
  int readTime();
  int getTime(int index) {return time[index];}
  void setTime(int index,int val) {time[index]=val;}
// Server processing
  void setAbort(bool a) { abort=a; condition.wakeOne(); }

// work with device
/*!
 * \brief Compleate read constant data
 *
 * Запись в файл блока данных настроек
 * \param[in] id
 * \param[in] fname
 * \return Code of erors
*/
  int initialDevice(void);

public slots:
  void timeAlarm(void);
private slots:
  void slotTimerEnable(bool);

protected:
    void run();

private:

// Status of device
  QString srvStatus; // status of server start error ready busy
  bool reInit;
  int address;
  QString hwVersion,hwStatus,hwError;
  QSerialPort *serial;
  QString serialPortName;
  int serialSpeed;
  QByteArray buf;
// process state machine
  CPhase phase;
  CPhase allStates[ALLREQSTATES];
  bool abort;
  QMutex mutex;
  QWaitCondition condition;
  int pastSt,presentSt;
// timers
  QTimer *tAlrm;

  int time[ALLVECTORS];

  int execCmd(QString command);
  int sendCmd(QString cmd);
  int readAnswer(QString& answer);
  int readStr(QString cmd,QString& ans);
  int readData(QString cmd,int ch,int *readData);
  int writeData(QString cmd,int ch, int data);

signals:
  void signalTimerEnable(bool);
  void signalMsg(QString,int);
  void signalDataReady(int);

};

#endif // HWBEHAVE_H
