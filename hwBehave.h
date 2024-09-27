#ifndef HWBEHAVE_H
#define HWBEHAVE_H
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

#define SAMPLE_DEVICE 1800 // time in ms for request device data (timer)

#define STARTING_STS "starting"
#define BUSY_STS "busy"
#define READY_STS "ready"
#define ERR_STS "error"

/**
*/
typedef enum
{
  GETSTATUS_STATE,
  WRITE_STATE,
  READ_STATE,
  TIMER_START_STATE,
  TIMER_STOP_STATE,
  ALLREQSTATES,    //limiter on process request states
    IDLE,
    READY,
    INITIAL_STATE,
    GETINFO_STATE,
    GLOBAL_ERROR_STATE,
    DEVICE_ERROR_STATE
}CPhase;  // phases of state machine

/**
 * @brief The TDtBehave class
 */
class THwBehave : public QThread
{
  Q_OBJECT
public:
  THwBehave();
  ~THwBehave();
  void readSettings(void);
// global error function


// working with HW
  void setErrorSt(short int); // set errors in state of statemachine for all stadies


// Server processing
  void setAbort(bool a) { abort=a; condition.wakeOne(); }


// work with device
  int initialDevice(void);
  int getInfoDevice();

public slots:
  void timeAlarm(void);

protected:
    void run();

private:

// Status of device
  QString srvStatus; // status of server start error ready busy
  bool reInit;


  QString serialPort;
  int serialSpeed;
// process state machine
  CPhase phase,nextPhase;
  CPhase allStates[ALLREQSTATES];
  bool abort,timerAlrm;
  QTimer *tAlrm;
  QMutex mutex;
  QWaitCondition condition;

signals:
  void disableNW_thread(THwBehave *);

};

#endif // HWBEHAVE_H
