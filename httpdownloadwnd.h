#ifndef HTTPDOWNLOADWND_H
#define HTTPDOWNLOADWND_H

#include <QNetworkAccessManager>
#include <QUrl>
#include <QWidget>

namespace Ui {
class HttpDownloadWnd;
}

class VersionData {
public:
  QString source = "";
  QString version = "0";
  QStringList urls;
};

class HttpDownloadWnd : public QWidget {
  Q_OBJECT

public:
  explicit HttpDownloadWnd(QWidget *parent = 0);
  ~HttpDownloadWnd();

private:
  QNetworkAccessManager accessManager;

private slots:
  void finishedGet(QNetworkReply *replay);

  void on_getButton_clicked();

private:
  const QString verFile = "version.json";
  const QString mainExe = "DJPlan.exe";

  void launchExe(const QString &exeName);

  bool saveToDisk(const QString &filename, QIODevice *data);
  QString calcFileName(const QUrl &url);

  // 解析 json 文件
  VersionData parseVersion(const QByteArray &json);

  // 本地版本
  VersionData getLocalVersion(const QString &filename);

  // 服务器最新版
  VersionData getRemoteVersion(const QString &url);

  // 下载文件到本地
  void downloadFile(const QString &url);

private:
  QList<QString> downloadList;

public:
  // 检查本地版本和服务器版本
  void doCheckVersion();

private:
  Ui::HttpDownloadWnd *ui;
};

#endif // HTTPDOWNLOADWND_H
