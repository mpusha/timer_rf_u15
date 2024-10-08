/*!
*  \file main_timerrf.h
*  \brief Заголовочный файл с описанием класса TTimerRf.
*/

#ifndef MAIN_TRF_H
#define MAIN_TRF_H

#include <qt5/QtWidgets/QtWidgets>
#include "hwBehave.h"

const double maxTime=59.500; ///< максимально возможное время уставки в мс.


class TTimerRf;
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
class TTimerRf : public QMainWindow
{
    Q_OBJECT

public:
//--- Constructor&Destructor ---
    TTimerRf(QWidget *parent = 0);
    ~TTimerRf();
    void putDataToTable(void);
    void getDataFromTable(void);

private:
//--- Widgets ---
  QVBoxLayout *main_layout;
  QHBoxLayout *edit_layout,*data_layout,*graph_layout;
  QGroupBox *MainGroupBox;            // Main Box
  QPushButton *update_btn,*write_btn;
  QLabel *space_Label,*err_Label,*status_Label,*hwver_Label;
  QTableWidget *tableRf;
  QMenu *file_menu;
  QAction *exit;
  QFont app_font;                 // Font

  QMessageBox msgBox;

private:
//--- Methods ---
    void create_ListWidget();
    void create_StatusBar();        // create StatusBar
    void create_Menu();             // create Menu
    void keyPressEvent(QKeyEvent *event);

private:
//--- Data ----
    QTranslator translator;
    QString LanguageApp;            // language application

    QTableWidgetItem *itemTable[1][ALLVECTORS];

    double data[1][ALLVECTORS];
    bool modifyData;
    THwBehave *dev;

public slots:
    void slot_writeData(void);
    void slot_processMsg(QString,int);
    void slot_updateHW(void);
    void slot_processData(int code);
};

#endif // MAIN_TRF_H
