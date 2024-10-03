#ifndef MAIN_TRF_H
#define MAIN_TRF_H


#include <qt5/QtWidgets/QtWidgets>
#include "hwBehave.h"

//------------------------------------------------------
#define MAXTIME 500
#define MINTIME 100
//#define ALLVECTORS 8

#define HEADER_COLOR  0xfff6f7f9

const double maxTime=59.500;

//------------------------------------------------------
class TTimerRf;
//------------------------------------------------------
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

  QTimer *timer,*alarmWrTimer;                  // Timer for time update
  QMessageBox msgBox;

private:
//--- Methods ---
    void create_ListWidget();
    void create_StatusBar();        // create StatusBar
    void create_Menu();             // create Menu
    void Sleep(int ms);
    void keyPressEvent(QKeyEvent *event);
//--- Widgets ---


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
    void slot_updateDateTime(void);
    void slot_alarmWriteAnswer(void);
    void slot_processMsg(QString,int);
    void slot_updateHW(void);
    void slot_processData(int code);

protected:


};

#endif // MAIN_TRF_H
