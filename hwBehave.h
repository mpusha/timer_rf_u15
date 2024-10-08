/*!
*  \file hwBehave.h
*  \brief Заголовочный файл с описанием класса THwBehave.
*/
#ifndef HWBEHAVE_H
#define HWBEHAVE_H
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

#define ALLVECTORS 8 ///< Число каналов таймера
#define SAMPLE_DEVICE 5000 ///< Время в мс опроса устройства
#define REP 3 ///< количество повторений при операциях чтения записи
#define SERIAL_TOUT 100 ///< таймаут мс на прием и передачу по последовательной линии
#define RS_DELAY 5 ///< задержка мс на переключение приемника/передатчика RS485

// коды ошибок, возвращаемые функциями
#define ERR_NONE 0 ///< нет ошибок
// from 0 to 7 reserv for uc
// 0 - отсутствие ошибки
// 1 - переполнение входного буфера (возможно посылаемвая строка не заканяивается кодом 0)
// 2 - формат входных данных не соответствует ожидаемому scanf
// 3 - ошибка во входных данных. Возможен выход из диапазона или отрицательный знак
// 4 - ошибка при записи в HW (считались данные не соответствующие записываемым)
#define ERR_UART_ABS 8     ///< отсутствует порт UART. Ошибка инициализации устройства
#define ERR_UART_TRANS 9   ///< ошибка отправки сообщения
#define ERR_UART_TOUT  10  ///< при приеме произоше таймаут UART_TIMEOUT
#define ERR_IDATA_CNT 11   ///< принято колличество данных менее ожидаемого
#define ERR_IDATA_ADDR 12  ///< приятый адрес не является числом
#define ERR_IDATA_CRET 13  ///< принятые данные не являются числом
#define ERR_IDATA_DATA 14  ///< принятые данные не совпадают с записываемыми (при операции проверки)
#define ERR_SETUP_ADDR 15  ///< адрес устройства не установлен, или выходит из диапазона 0-99
#define ERR_SETUP_CH 16    ///< устанавливаемый канал таймера выходит из диапазона 1-16, либо не установлен
#define ERR_SETUP_DATA 17  ///< устанавливаоемое время запуска или длительность выходного имульса выходит из диапазона MINDATA-MAXDATA MINWIDTH-MAXWIDTH
#define ERR_BAD        18  ///< получен код ошибки вместо 0
#define ERR_BADANSW    19  ///< получен неверный ответ (? символ)


/// набор состояний объекта. Вначале идут состояния, действия на которые запоминаются
typedef enum
{
  GETSTATUS_STATE, ///< получение статуса устройства
  WRITE_STATE, ///< запись данных в таймер
  READ_STATE, ///< чтение данных из таймера
  UPDATE_STATE, ///< обновление информации об таймере (версия, статус)
  ALLREQSTATES,    ///< ограничитель отложенных для исполнения действий
    IDLE,  ///< ожидание
    READY, ///< готовность
    INITIAL_STATE, ///<инициализация оборудования
    GETINFO_STATE, ///< запрос данных об устройстве
    GLOBAL_ERROR_STATE, ///< неустранимая ошибка, после которой необходим перезапуск
    SEND_STATE ///< отправка данных в графическое приложение
}CPhase;  // phases of state machine

/// Строки с названиями ошибок
const QString cErrStr[]={"none","input buffer overflow","scanf format","range input data is incorrect","HW write", //4
                         "none","none","none", //7
                         "UART port absent","can't send message","receiver timeout","count of input data is incorrect",//11
                         "address is incorrect", "time is incorrect","write/read data are different", //14
                         "address of device don't set","setup chanel of timer is outrange","setup time of timer is outrange","timer off",//18
                         "bad answer from uC"
                        }; //19

/*! \brief Класс для работы с модулем таймера системы "Шпат" ускорителя У-1.5
 *  \date Oct 2024
 * \author Sergey Sytov
 *
 * Класс включающий методы для работы с модулем таймера системы "Шпат" ускорителя У-1.5.
 * Методы позволяют произвести запись временных уставок в каналы таймера,
 * прочитать временные уставки, прочитать возникающие при работе ошибки и
 * статусы.
 *
*/
class THwBehave : public QThread
{
  Q_OBJECT
public:
  THwBehave();
  ~THwBehave();
  void readSettings(void);
  void setState(CPhase state);
  void setAbort(bool ab);
  int initialDevice(void);
  int readTime(void);
  int getTime(int index);
  void setTime(int index,int val);
  QString decodeErrorStr(int);

private:
  QSerialPort *serial; ///< последовательный порт
  QString serialPortName; ///< имя порта, по умолчанию /dev/ttyUSB0
  int serialSpeed; ///< скорость порта, по умолчантю 9600
  int address; ///< адрес таймера
  QString hwVersion,hwStatus,hwError; /// строки с версией статусом и ошибками, полученными из таймера

// process state machine and thread
  CPhase phase; ///< текущий шаг автомата состояний
  CPhase allStates[ALLREQSTATES]; ///< отложенный в выполнение шаг автомата сотояний
  bool abort; ///< переменная по которой осуществляется выход из процесса обработки событий
  QMutex mutex; ///< блокирующий мьютекс
  QWaitCondition condition; ///< управление потоком
  int pastSt,presentSt; ///< прошлое и настоящее состояние обнаружения таймера

  QTimer *tAlrm; ///< внутренний таймер, по которому осуществляется опрос внешнего устройства

  int time[ALLVECTORS]; ///< массив данных со временами срабатывания таймера в мкс

private:
  int execCmd(QString command);
  int sendCmd(QString cmd);
  int readAnswer(QString& answer);
  int readStr(QString cmd,QString& ans);
  int readData(QString cmd,int ch,int *readData);
  int writeData(QString cmd,int ch, int data);

protected:
    void run();

public slots:
  void slotTimeAlarm(void);
  void slotTimerEnable(bool);

signals:
  void signalTimerEnable(bool);///< Сигнал на разрешение работы внутреннего таймера
  void signalMsg(QString,int);///< Сигнал в GUI поток с сообщениями об ошибках и статусом
  void signalDataReady(int);///< Сигнал о готовности данных

};

#endif // HWBEHAVE_H
