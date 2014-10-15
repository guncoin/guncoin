#ifndef MININGPAGE_H
#define MININGPAGE_H

#include "clientmodel.h"

#include <QWidget>

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTime>
#include <QTimer>
#include <QStringList>
#include <QMap>
#include <QSettings>

// Log types
#define STARTED 0
#define SHARE_SUCCESS 1
#define SHARE_FAIL 2
#define ERROR 3
#define LONGPOLL 4
#define STRATUM 5

namespace Ui {
    class MiningPage;
}
class ClientModel;

class MiningPage : public QWidget
{
    Q_OBJECT

public:
    explicit MiningPage(QWidget *parent = 0);
    ~MiningPage();

    bool minerActive;

    QProcess *minerProcess;

    QMap<int, double> threadSpeed;

    QTimer *readTimer;

    uint acceptedShares;
    uint rejectedShares;

    uint roundAcceptedShares;
    uint roundRejectedShares;

    uint initThreads;

    void setModel(ClientModel *model);

public slots:
    void startPressed();

    void minerStarted();
    void minerFinished();

    void startPoolMining();
    void stopPoolMining();

    void saveSettings();
    void loadSettings();

    void readProcessOutput();
    void minerError(QProcess::ProcessError);
    void updateSpeed();
    void reportToList(QString, int, QString);
    QString getTime(QString);

    void setMiningControls(uint mode);
    ClientModel::MiningType getMiningType();
    void typeChanged(int index);
    void debugToggled(bool checked);

private slots:

private:
    Ui::MiningPage *ui;
    ClientModel *model;

    void resetMiningButton();
};

#endif // MININGPAGE_H
