#include <QApplication>
#include "main_timerrf.h"

//! Основная программа
int main(int argc, char *argv[])
{

    QApplication a(argc, argv);
    // setup locale loc get from settings file
    /*QTranslator translator(0);
    QString loc;
    QSettings setings(QApplication::applicationDirPath()+"/setup.ini",QSettings::IniFormat) ;
    loc=setings.value("Locale","EN").toString();
    if(!loc.compare("auto",Qt::CaseInsensitive))
      translator.load(QApplication::applicationDirPath()+QString("/rfamp_")+QLocale::system().name(),".");
     else
       if(!loc.compare("ru",Qt::CaseInsensitive))
         translator.load("rfamp_ru",".");
        else
          translator.load("rfamp_en",".");
    a.installTranslator(&translator);
    */
    a.setApplicationVersion(APP_VERSION);
    TTimerRf rf;

    rf.show();

    return a.exec();
}
