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
#define ERR_UART_TOUT  10  // при приеме произоше таймаут UART_TIMEOUT определенный в serial.h
#define ERR_IDATA_CNT 11   // принято колличество данных менее ожидаемого
#define ERR_IDATA_ADDR 12  // приятый адрес не является числом
#define ERR_IDATA_CRET 13  // принятые данные не являются числом
#define ERR_IDATA_DATA 14  // принятые данные не совпадают с записываемыми (при операции проверки)
#define ERR_SETUP_ADDR 15  // адрес устройства не установлен, или выходит из диапазона 0-99
#define ERR_SETUP_CH 16    // устанавливаемый канал таймера выходит из диапазона 1-16, либо не установлен
#define ERR_SETUP_DATA 17  // устанавливаоемое время запуска или длительность выходного имульса выходит из диапазона MINDATA-MAXDATA MINWIDTH-MAXWIDTH
#define ERR_FILE_WRITE 18  // ошибка открытия файла на запись при чтении данных из таймера
#define ERR_FILE_READ 19   // ошибка открытия файла на чтение при записи данных в таймер
#define ERR_FILE_SETUP_ABS 20 // отсутствует файл setup.xms in settings
#define ERR_FILE_SETTINGS 21  // ошибка в файле с установками либо он отсутстве

#define TADDR 1
#define SERIAL_TOUT 500
#define UART_SHORT_TOUT 50L
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
  int testAlive(void);
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
private slots:
  void slotTimerEnable(bool);

protected:
    void run();

private:

// Status of device
  QString srvStatus; // status of server start error ready busy
  bool reInit;
  int address;
  QSerialPort *serial;
  QString serialPortName;
  int serialSpeed;
// process state machine
  CPhase phase;
  CPhase allStates[ALLREQSTATES];
  bool abort;
  QTimer *tAlrm;
  QMutex mutex;
  QWaitCondition condition;

signals:
  void disableNW_thread(THwBehave *);
  void signalTimerEnable(bool);
  void signalMsg(QString,int);

};

#endif // HWBEHAVE_H
