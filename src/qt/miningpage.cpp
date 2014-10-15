#include "miningpage.h"
#include "ui_miningpage.h"
#include "util.h"

MiningPage::MiningPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MiningPage)
{
    ui->setupUi(this);

    setFixedSize(400, 420);

    minerActive = false;

    minerProcess = new QProcess(this);
    minerProcess->setProcessChannelMode(QProcess::MergedChannels);

    readTimer = new QTimer(this);

    acceptedShares = 0;
    rejectedShares = 0;

    roundAcceptedShares = 0;
    roundRejectedShares = 0;

    initThreads = 0;

    connect(readTimer, SIGNAL(timeout()), this, SLOT(readProcessOutput()));

    connect(ui->startButton, SIGNAL(pressed()), this, SLOT(startPressed()));
    connect(ui->typeBox, SIGNAL(currentIndexChanged(int)), this, SLOT(typeChanged(int)));
    connect(ui->debugCheckBox, SIGNAL(toggled(bool)), this, SLOT(debugToggled(bool)));
    connect(minerProcess, SIGNAL(started()), this, SLOT(minerStarted()));
    connect(minerProcess, SIGNAL(error(QProcess::ProcessError)), this, SLOT(minerError(QProcess::ProcessError)));
    connect(minerProcess, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(minerFinished()));
    connect(minerProcess, SIGNAL(readyRead()), this, SLOT(readProcessOutput()));

    setModel(new ClientModel(0, this));
}

MiningPage::~MiningPage() {
    minerProcess->kill();

    delete(ui);
}

void MiningPage::setModel(ClientModel *model) {
    this->model = model;

    loadSettings();

    bool pool = model->getMiningType() == ClientModel::PoolMining;
    ui->threadsBox->setValue(model->getMiningThreads());
    ui->typeBox->setCurrentIndex(pool ? 0 : 1);
}

void MiningPage::startPressed() {
    initThreads = ui->threadsBox->value();

    if(minerActive == false) {
        saveSettings();

        if(getMiningType() == ClientModel::SoloMining)
          minerStarted();
        else
          startPoolMining();
    } else {
        if(getMiningType() == ClientModel::SoloMining)
          minerFinished();
        else
          stopPoolMining();
    }
}

void MiningPage::minerStarted() {
    if(!minerActive) {
        if(getMiningType() == ClientModel::SoloMining) {
            reportToList("Solo mining started.", ERROR, NULL);
            ui->mineSpeedLabel->setText("Speed: N/A");
            readTimer->start(500);
        } else {
          reportToList("Miner started. You might not see any output for a few minutes.", STARTED, NULL);
        }
    }
    minerActive = true;
    resetMiningButton();
    model->setMining(getMiningType(), true, initThreads, 0);
}

void MiningPage::minerFinished() {
    if(getMiningType() == ClientModel::SoloMining) {
        reportToList("Solo mining stopped.", ERROR, NULL);
        ui->mineSpeedLabel->setText("");
        readTimer->stop();
    } else {
        reportToList("Miner exited.", ERROR, NULL);
    }
    ui->list->addItem("");
    minerActive = false;
    resetMiningButton();
    model->setMining(getMiningType(), false, initThreads, 0);
}

void MiningPage::startPoolMining() {
    QStringList args;
    QString url = ui->serverLine->text();
    if(!url.contains("http://") && !url.contains("stratum+tcp://"))
      url.prepend("stratum+tcp://");
    QString urlLine = QString("%1:%2").arg(url, ui->portLine->text());
    QString userpassLine = QString("%1:%2").arg(ui->usernameLine->text(), ui->passwordLine->text());
    args << "-a" << (fNeoScrypt ? "neoscrypt" : "scrypt");
    args << "-s" << ui->scantimeBox->text().toAscii();
    args << "-o" << urlLine.toAscii();
    args << "-O" << userpassLine.toAscii();
    args << "-t" << ui->threadsBox->text().toAscii();
    args << "-r" << "-1"; // Retry forever.
    args << "-P"; // This is needed for this to work correctly on Windows. Extra protocol dump helps flush the buffer quicker.

    threadSpeed.clear();

    acceptedShares = 0;
    rejectedShares = 0;

    roundAcceptedShares = 0;
    roundRejectedShares = 0;

    /* Assume minerd is in the current path first and in a system path second */
    QString program = QDir::current().filePath("minerd");
    if(!QFile::exists(program))
      program = "minerd";

    if(ui->debugCheckBox->isChecked())
      ui->list->addItem(args.join(" ").prepend(" ").prepend(program));

    ui->mineSpeedLabel->setText("Speed: N/A");
    ui->shareCount->setText("Accepted: 0   Rejected: 0");
    minerProcess->start(program,args);
    minerProcess->waitForStarted(-1);

    readTimer->start(500);
}

void MiningPage::stopPoolMining() {
    ui->mineSpeedLabel->setText("");
    ui->shareCount->setText("");
    minerProcess->kill();
    readTimer->stop();
}

void MiningPage::saveSettings() {
    model->setMiningDebug(ui->debugCheckBox->isChecked());
    model->setMiningScanTime(ui->scantimeBox->value());
    model->setMiningServer(ui->serverLine->text());
    model->setMiningPort(ui->portLine->text());
    model->setMiningUsername(ui->usernameLine->text());
    model->setMiningPassword(ui->passwordLine->text());
}

void MiningPage::loadSettings() {
    ui->debugCheckBox->setChecked(model->getMiningDebug());
    ui->scantimeBox->setValue(model->getMiningScanTime());
    ui->serverLine->setText(model->getMiningServer());
    ui->portLine->setText(model->getMiningPort());
    ui->usernameLine->setText(model->getMiningUsername());
    ui->passwordLine->setText(model->getMiningPassword());
}

void MiningPage::readProcessOutput() {

    if(getMiningType() == ClientModel::SoloMining) {
        ui->mineSpeedLabel->setText(QString("Speed: %1.%2 KH/s   Threads: %3")
          .arg(nMiningSpeed / 1000).arg(nMiningSpeed % 1000).arg(nMiningThreads));
        return;
    }

    QByteArray output;

    minerProcess->reset();

    output = minerProcess->readAll();

    QString outputString(output);

    if(!outputString.isEmpty()) {

        int i;
        QStringList list = outputString.split("\n", QString::SkipEmptyParts);
        for(i = 0; i < list.size(); i++) {
            QString line = list.at(i);

            if(ui->debugCheckBox->isChecked()) {
                ui->list->addItem(line.trimmed());
                ui->list->scrollToBottom();
            }

            if(line.contains("(yay!!!)"))
              reportToList("Share accepted!", SHARE_SUCCESS, getTime(line));
            else if(line.contains("(booooo)"))
              reportToList("Share rejected", SHARE_FAIL, getTime(line));
            else if(line.contains("LONGPOLL detected new block"))
              reportToList("Long polling detected a new block", LONGPOLL, getTime(line));
            else if(line.contains("Stratum detected new block"))
              reportToList("Stratum detected a new block", STRATUM, getTime(line));
            else if(line.contains("Long-polling activated"))
              reportToList("Long polling activated.", ERROR, NULL);
            else if(line.contains("Supported options:"))
              reportToList("Miner didn't start properly. Try checking your settings.", ERROR, NULL);
            else if(line.contains("HTTP request failed"))
              reportToList("Couldn't connect via HTTP. Please check the server name and port.", ERROR, NULL);
            else if(line.contains("stratum_subscribe time out"))
              reportToList("Couldn't connect via Stratum. Please check the server name and port.", ERROR, NULL);
            else if(line.contains("Bad worker credentials"))
              reportToList("Authentication failed via HTTP. Please check your username and password.", ERROR, NULL);
            else if(line.contains("Stratum authentication failed"))
              reportToList("Authentication failed via Stratum. Please check your username and password.", ERROR, NULL);
            else if(line.contains("retry after 30 seconds"))
              reportToList("Couldn't communicate with the server. Retrying in 30 seconds.", ERROR, NULL);
            else if(line.contains("thread ") && line.contains("khash/s")) {
                QString threadIDstr = line.at(line.indexOf("thread ") + 7);
                int threadID = threadIDstr.toInt();

                int threadSpeedindx = line.indexOf(",");
                QString threadSpeedstr = line.mid(threadSpeedindx);
                threadSpeedstr.chop(8);
                threadSpeedstr.remove(", ");
                threadSpeedstr.remove(" ");
                threadSpeedstr.remove('\n');
                double speed = 0;
                speed = threadSpeedstr.toDouble();

                threadSpeed[threadID] = speed;

                updateSpeed();
            }
        }
    }
}

void MiningPage::minerError(QProcess::ProcessError error) {
    if(error == QProcess::FailedToStart) {
        reportToList("Miner failed to start. Make sure you have the minerd executable in the same directory as Phoenixcoin-Qt.", ERROR, NULL);
    }
}

void MiningPage::updateSpeed() {
    uint totalThreads = 0;
    double totalSpeed = 0;

    QMapIterator<int, double> iter(threadSpeed);
    while(iter.hasNext()) {
        iter.next();
        totalSpeed += iter.value();
        totalThreads++;
    }

    // If all threads haven't reported the hash speed yet, make an assumption
    if(totalThreads != initThreads)
      totalSpeed = (totalSpeed / totalThreads) * initThreads;

    QString speedString = QString("%1").arg(totalSpeed);
    QString threadsString = QString("%1").arg(initThreads);

    QString acceptedString = QString("%1").arg(acceptedShares);
    QString rejectedString = QString("%1").arg(rejectedShares);

    QString roundAcceptedString = QString("%1").arg(roundAcceptedShares);
    QString roundRejectedString = QString("%1").arg(roundRejectedShares);

    ui->shareCount->setText(QString("Accepted: %1 (%3)   Rejected: %2 (%4)")
      .arg(acceptedString, rejectedString, roundAcceptedString, roundRejectedString));
    ui->mineSpeedLabel->setText(QString("Speed: %1 KH/s   Threads: %2")
      .arg(speedString, threadsString));

    model->setMining(getMiningType(), true, initThreads, (uint)(totalSpeed * 1000));
}

void MiningPage::reportToList(QString msg, int type, QString time) {
    QString message;
    if(time == NULL)
      message = QString("[%1] - %2").arg(QTime::currentTime().toString(), msg);
    else
      message = QString("[%1] - %2").arg(time, msg);

    switch(type) {

        case(SHARE_SUCCESS):
            acceptedShares++;
            roundAcceptedShares++;
            updateSpeed();
            break;

        case(SHARE_FAIL):
            rejectedShares++;
            roundRejectedShares++;
            updateSpeed();
            break;

        case(LONGPOLL):
            roundAcceptedShares = 0;
            roundRejectedShares = 0;
            break;

        case(STRATUM):
            roundAcceptedShares = 0;
            roundRejectedShares = 0;
            break;

        default:
            break;

    }

    ui->list->addItem(message);
    ui->list->scrollToBottom();
}


// Function for fetching the time
QString MiningPage::getTime(QString time) {

    if(time.contains("[")) {
        time.resize(21);
        time.remove("[");
        time.remove("]");
        time.remove(0,11);
        return(time);
    }

    return(NULL);
}

/* 0 = disable all controls;
 * 1 = enable pool controls;
 * 2 = enable solo controls */
void MiningPage::setMiningControls(uint mode) {
    bool a, b;

    if(mode == 2) {
        a = true;
        b = false;
    }
    else if(mode) a = b = true;
    else a = b = false; 

    ui->typeBox->setEnabled(a);
    ui->threadsBox->setEnabled(a);
    ui->scantimeBox->setEnabled(b);
    ui->serverLine->setEnabled(b);
    ui->portLine->setEnabled(b);
    ui->usernameLine->setEnabled(b);
    ui->passwordLine->setEnabled(b);
}

ClientModel::MiningType MiningPage::getMiningType() {
    if(ui->typeBox->currentIndex() == 1)
      return(ClientModel::SoloMining);
    else
      return(ClientModel::PoolMining);
}

/* 0 = pool mining selected;
 * 1 = solo mining selected */
void MiningPage::typeChanged(int index) {
    if(index == 1)
      setMiningControls(2);
    else
      setMiningControls(1);
}

void MiningPage::debugToggled(bool checked) {
    model->setMiningDebug(checked);
}

void MiningPage::resetMiningButton() {
    ui->startButton->setText(minerActive ? "Stop Mining" : "Start Mining");
    if(minerActive) setMiningControls(0);
    else {
        if(getMiningType() == ClientModel::SoloMining)
          setMiningControls(2);
        else
          setMiningControls(1);
    }
}

