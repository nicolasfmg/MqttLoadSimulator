#ifndef CLIENTPOOL_H
#define CLIENTPOOL_H

#include <QObject>
#include <oneclient.h>

class ClientPool : public QObject
{
    Q_OBJECT

    QList<OneClient*> clients;
public:
    explicit ClientPool(QString hostname, quint16 port, QString username, QString password, bool pub_and_sub, int amount, QString clientIdPart, QObject *parent = nullptr);
    ~ClientPool();



signals:

public slots:
    void startClients();
};

#endif // CLIENTPOOL_H
