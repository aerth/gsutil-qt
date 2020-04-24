#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QTranslator>

const QString VersionStr = "gsutil 1.0.0";
const QString DefaultRemoteHost =
    QString(getenv("DEFAULTBUCKET"));  // export DEFAULTBUCKET="yourbucket_name"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

 private slots:
  void on_actionVersionString_triggered();

  void on_actionQuit_triggered();

  void on_btn_chooseFile_clicked();

  void on_editFilename_textChanged(const QString &arg1);

  void on_btn_upload_clicked();

 private:
  QString remoteHost = DefaultRemoteHost;
  Ui::MainWindow *ui;
  void doThing(void);
  bool installGsutil(void);
};

#endif  // MAINWINDOW_H
