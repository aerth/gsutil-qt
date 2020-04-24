#include "mainwindow.h"
#include <QDebug>
#include <QFileDialog>
#include <QProcess>
#include "ui_mainwindow.h"
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  if (system("which gsutil > /dev/null 2>&1")) {
    QMessageBox::critical(this, "gsutil not found",
                          "You don't have 'gsutil' installed. Do that first.");
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Install gs-util", "Install gs-util?",
                                  QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) {
      qDebug() << "Yes was *not* clicked, exiting";
      exit(111);
    }
    qDebug() << "Yes was clicked, installing gsutil from "
                "https://storage.googleapis.com/pub/gsutil.tar.gz";
    if (!this->installGsutil()) {
      QMessageBox::critical(this, "installation error",
                            "installation of gsutil failed. please try again "
                            "(or install manually)");
      QApplication::quit();
      exit(111);
    }
    QMessageBox::information(nullptr, "Installation successful",
                             "Installation was successful, now open a shell "
                             "and type: \"bin/gsutil config\" to authenticate");
    QApplication::quit();
    exit(0);
  }

  if (this->remoteHost.length() == 0) {
    QMessageBox msgbox;
    QGridLayout *layout = (QGridLayout *)msgbox.layout();
    layout->setColumnMinimumWidth(1, 350);
    msgbox.setText(
        "<div style=\"font-size: 12px;\">"
        "Please set the env var: DEFAULTBUCKET "
        "to the name of your bucket.<br>"
        "Example:<br><br>"
        "<code>echo export DEFAULTBUCKET=foo_bar >> ~/.bashrc<br>"
        "source ~/.bashrc</code>"
        "<br></div>");
    msgbox.setWindowTitle("Please set DEFAULTBUCKET env var");
    msgbox.exec();
    QApplication::quit();
    exit(111);
  }

  ui->setupUi(this);
  ui->actionVersionString->setText(VersionStr);
  ui->radioPrivate->setChecked(true);
  this->ui->btn_upload->setEnabled(false);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::on_actionVersionString_triggered() {
  const QString AboutText(
      tr("Welcome to version %1\nsource code: %2")
          .arg(VersionStr, "https://github.com/aerth/gsutil-qt"));
  QMessageBox::information(nullptr, VersionStr, AboutText);
}

void MainWindow::on_actionQuit_triggered() { exit(0); }

void MainWindow::on_btn_chooseFile_clicked() {
  QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "");
  this->ui->editFilename->setText(fileName);
}

void MainWindow::on_editFilename_textChanged(const QString &arg1) {
  if (arg1.length() != 0) {
    this->ui->editRemotename->setText(
        tr("gs://%1/%2").arg(this->remoteHost, QFileInfo(arg1).fileName()));
    this->ui->btn_upload->setEnabled(true);
  } else {
    this->ui->btn_upload->setEnabled(false);
  }
}

void MainWindow::on_btn_upload_clicked() {
  QProcess *process = new QProcess(this);
  process->setProcessChannelMode(QProcess::MergedChannels);
  QString gsflags(tr("-a %1").arg(
      this->ui->radioPublic->isChecked() ? "public-read" : "private"));
  QString remoteName(this->ui->editRemotename->text());
  QString inputName(this->ui->editFilename->text());
  if (inputName.length() == 0) {
    return;
  }

  connect(process,
          QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          [=](int exitCode, QProcess::ExitStatus exitStatus) {
            qDebug() << "hello";
            QString output = process->readAllStandardOutput();
            QString outputerr = process->readAllStandardError();

            ui->progressBar->reset();
            ui->progressBar->setMaximum(1);
            ui->progressBar->setMinimum(0);
            ui->statusBar->showMessage("Finished Uploading.");
            ui->progressBar->setEnabled(false);
            if (exitCode != 0) {
              QMessageBox::critical(
                  this, "Error",
                  QString(tr("Exit code %1\n%2\n%3\n%4")
                              .arg(QString::number(exitCode), output, outputerr,
                                   QString(exitStatus))));
              exit(111);
            }
            QString link(tr("\n<a "
                            "href=\"https://storage.googleapis.com%1\">https://"
                            "storage.googleapis.com%1</a>")
                             .arg(remoteName.mid(4, remoteName.length())));
            QMessageBox::information(nullptr, VersionStr,
                                     output + "\n" + outputerr + link);
          });
  process->start(tr("gsutil cp %3 %1 %2")
                     .arg(this->ui->editFilename->text(), remoteName, gsflags));

  if (!process->waitForStarted()) {
    QMessageBox::critical(this, "Error", "could not start");
    return;
  }

  // start spinning!
  ui->progressBar->setEnabled(true);
  connect(process, &QProcess::readyRead, this, [=](void) {
    QTextStream stream(process);
    QString s = stream.readLine();
    ui->statusBar->showMessage("Uploading..... " + s);
  });
  // while running
  if (process->state() == QProcess::Running) {
    static int count = 0;
    ui->progressBar->setMaximum(0);
    ui->progressBar->setMinimum(0);
    ui->progressBar->setValue(count++);
  }
}
bool MainWindow::installGsutil(void) {
  QString remoteUrl = "https://storage.googleapis.com/pub/gsutil.tar.gz";
  QProcess *process = new QProcess();
  process->setProcessChannelMode(QProcess::MergedChannels);
  process->start("wget -O /tmp/gsutil.tar.gz " + remoteUrl);
  process->waitForFinished(-1);
  if (process->exitCode() != 0) {
    QMessageBox::critical(this, "Error Downloading", process->readAll());
    return false;
  }
  qDebug() << "wget success";
  QString homedir(QString(getenv("HOME")));
  process->start(tr("tar xaf /tmp/gsutil.tar.gz -C %1").arg(homedir));
  process->waitForFinished(-1);
  if (process->exitCode() != 0) {
    QMessageBox::critical(this, "Error Extracting", process->readAll());
    return false;
  }
  qDebug() << "tar success";
  QString linkStr = QString("ln -s -f " + homedir + "/gsutil/gsutil " +
                            homedir + "/bin/gsutil");
  process->start(linkStr);
  process->waitForFinished(-1);
  if (process->exitCode() != 0) {
    QMessageBox::critical(this, "Error Symlinking", process->readAll());
    return false;
  }
  qDebug() << "symlink success";
  std::string path(getenv("PATH"));

  path.append(":" + homedir.toStdString() + "/bin");
  setenv("PATH", path.c_str(), 1);
  return true;
}
