//#include "mainwindow.h"
#include "httpdownloadwnd.h"
#include <QApplication>

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);
  HttpDownloadWnd w;
  w.doCheckVersion();

  //  w.show();

  return a.exec();
}
