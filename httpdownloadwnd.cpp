#include "httpdownloadwnd.h"
#include "ui_httpdownloadwnd.h"

#include <QFile>
#include <QFileInfo>
#include <QNetworkReply>
#include <QProcess>
#include <QtWidgets>

HttpDownloadWnd::HttpDownloadWnd(QWidget *parent)
    : QWidget(parent), ui(new Ui::HttpDownloadWnd) {
  ui->setupUi(this);

  // 异步
  connect(&accessManager, &QNetworkAccessManager::finished, this,
          &HttpDownloadWnd::finishedGet);
}

HttpDownloadWnd::~HttpDownloadWnd() { delete ui; }

void HttpDownloadWnd::finishedGet(QNetworkReply *reply) {
  // 把 reply 中数据，写入到 url 对应的本地文件中
  QUrl url = reply->url();
  qDebug() << url.toString();

  if (reply->error()) {
    qDebug() << "can not download from " << url.toString();
  } else {
    QString filename = calcFileName(url);
    if (saveToDisk(filename, reply)) {
      printf("Download of %s succeeded (saved to %s)\n",
             url.toEncoded().constData(), qPrintable(filename));
    }

    // 启动下载时，加入到下载列表，这里下载完毕了，从列表中移除；
    downloadList.removeAll(url.toString());

    reply->deleteLater();
  }

  if (downloadList.isEmpty()) {
    // 所有需要下载的文件都已经下载了，关闭下载程序，启动主程序；
    launchExe(mainExe);
  }
}

void HttpDownloadWnd::on_getButton_clicked() {
  //  QString url = ui->leUrl->text().trimmed();
  //  QNetworkRequest request(url);
  //  accessManager.get(request);

  doCheckVersion();
}

void HttpDownloadWnd::launchExe(const QString &exeName) {
  // 所有需要下载的文件都已经下载了，关闭下载程序，启动主程序；
  QProcess::startDetached(exeName);
  qApp->exit();
}

bool HttpDownloadWnd::saveToDisk(const QString &filename, QIODevice *data) {
  QFile file(filename);
  if (!file.open(QIODevice::WriteOnly)) {
    qDebug() << "can not open: " << filename
             << " error: " << file.errorString();
    return false;
  }

  file.write(data->readAll());
  file.close();

  return true;
}

QString HttpDownloadWnd::calcFileName(const QUrl &url) {
  QString path = url.path();
  qDebug() << "url: " << url << " \n path: " << path;

  QString basename = QFileInfo(path).fileName();
  qDebug() << "basename: " << basename;

  if (basename.isEmpty()) {
    basename = "download";
  }

  // 升级版本，直接覆盖，不需要判断文件重复
  return basename;
}

VersionData HttpDownloadWnd::parseVersion(const QByteArray &byteArr) {
  // 解析 version.json
  // 取得 版本，文件列表；
  const QString source = "source";
  const QString version = "version";
  const QString files = "files";

  VersionData data;

  QJsonParseError jsonError;
  QJsonDocument doucment = QJsonDocument::fromJson(byteArr, &jsonError);

  if (!doucment.isNull() && (jsonError.error == QJsonParseError::NoError)) {
    if (doucment.isObject()) {
      QJsonObject object = doucment.object();

      // 读取 source 的 value
      if (object.contains(source)) {
        QJsonValue valSource = object.value(source);
        if (valSource.isString()) {
          data.source = valSource.toString();
          qDebug() << "source : " << data.source;
        }
      }

      // 读取 version 的 value
      if (object.contains(version)) {
        QJsonValue value = object.value(version);
        if (value.isString()) {
          data.version = value.toString();
          qDebug() << "version : " << data.version;
        }
      }

      // 读取 files 的 value
      // 是数组，把里面的每一项，都加到 list 中；
      if (object.contains(files)) {
        QJsonValue value = object.value(files);
        if (value.isArray()) {
          QJsonArray array = value.toArray();

          for (const QJsonValue &value : array) {
            if (value.type() == QJsonValue::String) {
              QString fileName = value.toString().trimmed();
              qDebug() << fileName;

              data.urls.append(fileName);
            }
          }
        }
      }
    }
  }

  return data;
}

VersionData HttpDownloadWnd::getRemoteVersion(const QString &url) {
  // 从指定 url 读取
  //  QNetworkRequest request(QUrl(url));
  //  QNetworkReply *reply = accessManager.get(request);
  qDebug() << "fetch: " << url;

  // 同步请求，需要一个新的 accessManager
  QNetworkAccessManager syncManager;
  QNetworkReply *reply = syncManager.get(QNetworkRequest(QUrl(url)));

  // 同步等待
  QEventLoop loop;
  connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  loop.exec(QEventLoop::ExcludeUserInputEvents);

  QByteArray response_data = reply->readAll();
  //  qDebug() << "remote data: " << response_data;

  VersionData data = parseVersion(response_data);

  reply->deleteLater();

  return data;
}

VersionData HttpDownloadWnd::getLocalVersion(const QString &filename) {
  // 从指定文件读取；
  VersionData data;
  QFile file(filename);

  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug() << "can not load verion file.";
    return data;
  }

  QByteArray byteArr = file.readAll();
  file.close();

  data = parseVersion(byteArr);
  return data;
}

void HttpDownloadWnd::doCheckVersion() {
  VersionData v1 = getLocalVersion(verFile);

  // files 不包含 servername, port, 但是，下载文件需要，所以再加上；
  QString baseURI = QUrl(v1.source).toString(QUrl::RemovePath);
  //  qDebug() << "source: " << v1.source << "\n baseURI" << baseURI;

  VersionData remote = getRemoteVersion(v1.source);

  if (v1.version.compare(remote.version) < 0) {
    // 本地版版本 小于 服务器版本
    // 从服务器把文件都更新到本地目录
    for (const QString &url : remote.urls) {
      QString fullName = baseURI + url;
      downloadList.append(fullName);
      downloadFile(fullName);
    }
  } else {
    launchExe(mainExe);
  }
}

void HttpDownloadWnd::downloadFile(const QString &url) {
  QNetworkRequest request(url);
  accessManager.get(request);
}

#include "moc_httpdownloadwnd.cpp"
