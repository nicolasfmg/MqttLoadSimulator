#include "oneclient.h"
#include "utils.h"
#include "iostream"
#include <QSslConfiguration>

#include "globals.h"
#include "clientnumberpool.h"

thread_local QHash<QString, QHostInfo> OneClient::dnsCache;

OneClient::OneClient(const QString &hostname, quint16 port, const QString &username, const QString &password, bool pub_and_sub, int clientNr, const QString &clientIdPart,
                     bool ssl, const QString &clientPoolRandomId, const int totalClients, const int delay, int burst_interval, const uint burst_spread,
                     int burst_size, int overrideReconnectInterval, const QString &topic, uint qos, bool incrementTopicPerPublish,
                     const QString &clientid, bool cleanSession, QObject *parent) :
    QObject(parent),
    client_id(!clientid.isEmpty() ? clientid : QString("%1_%2_%3_%4").arg(QHostInfo::localHostName()).arg(clientIdPart).arg(clientNr).arg(GetRandomString())),
    clientNr(clientNr),
    pub_and_sub(pub_and_sub),
    publishTimer(this),
    clientPoolRandomId(clientPoolRandomId),
    burstSize(burst_size),
    topicBase(topic),
    payloadBase(QString("Client %1 publish counter: %2").arg(client_id)),
    qos(qos),
    incrementTopicPerPublish(incrementTopicPerPublish)
{
    if (ssl)
    {
        QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);

        this->client = new QMQTT::Client(hostname, port, sslConfig, true);
    }
    else
    {
        if (!dnsCache.contains(hostname))
        {
            dnsCache[hostname] = QHostInfo::fromName(hostname);
        }

        // Ehm, why the difference in QMTT::Client's overloaded constructors for SSL and non-SSL?
        this->client = new QMQTT::Client(dnsCache[hostname].addresses().first(), port);
    }

    if (!topic.isEmpty())
    {
        if (topic.contains("%1"))
        {
            int nr = ClientNumberPool::getClientNr();
            subscribeTopic = QString(topic).arg(nr);

            if (pub_and_sub)
                publishTopic = subscribeTopic;
        }
        else
        {
            subscribeTopic = topic;
            publishTopic = topic;
        }
    }
    else
    {
        if (pub_and_sub)
        {
            publishTopic = QString("loadtester/clientpool_%1/%2/hellofromtheloadtester").arg(this->clientPoolRandomId).arg(this->clientNr);
            subscribeTopic = QString("loadtester/clientpool_%1/%2/#").arg(this->clientPoolRandomId).arg(this->clientNr - 1);
        }
        else
        {
            QString ran = GetRandomString();
            subscribeTopic = QString("silentpath/%1/#").arg(ran);
        }
    }

    QString u = username;
    if (username.contains("%1"))
    {
        this->usernameBase = username;
        regenRandomUsername = true;
        u = QString(this->usernameBase).arg(GetRandomString());
    }

    QString p = password;
    if (password.contains("%1"))
    {
        this->passwordBase = password;
        regenRandomPassword = true;
        p = QString(this->passwordBase).arg(GetRandomString());
    }

    client->setClientId(client_id);
    client->setUsername(u);
    client->setPassword(p.toLatin1());
    client->setCleanSession(cleanSession);

    int keepAlive = 60;
    client->setKeepAlive(keepAlive);

    connect(client, &QMQTT::Client::connected, this, &OneClient::connected);
    connect(client, &QMQTT::Client::disconnected, this, &OneClient::onDisconnect);
    connect(client, &QMQTT::Client::error, this, &OneClient::onClientError);
    connect(client, &QMQTT::Client::received, this, &OneClient::onReceived);

    int spread = burst_spread/2 - (qrand() % burst_spread);
    int interval = burst_interval + spread;
    interval = std::max<int>(1, interval);

    publishTimer.setInterval(interval);
    publishTimer.setSingleShot(false);
    connect(&publishTimer, &QTimer::timeout, this, &OneClient::onPublishTimerTimeout);

    const int totalConnectionDuration = ((totalClients + 1) / (1000.0 / (delay + 1))) * 1000;
    const int reconnectInterval = overrideReconnectInterval >= 0 ? overrideReconnectInterval : 5000 + (qrand() % totalConnectionDuration);
    reconnectTimer.setInterval(reconnectInterval);
    reconnectTimer.setSingleShot(true);
    connect(&reconnectTimer, &QTimer::timeout, this, &OneClient::connectToHost);
}

OneClient::~OneClient()
{
    if (client)
    {
        client->disconnectFromHost();
        delete client;
        client = nullptr;
    }
}

Counters OneClient::getCounters() const
{
    return counters;
}

void OneClient::connectToHost()
{
    if (!_connected) // client->isConnectedToHost() checks the wrong thing (whether socket is connected), and is true when SSL is still being negotiated.
    {
        if (Globals::verbose)
            std::cout << "Connecting...\n";
        client->connectToHost();
    }
}

void OneClient::connected()
{
    _connected = true;
    counters.connect++;

    if (Globals::verbose)
        std::cout << "Connected.\n";

    if (this->pub_and_sub)
    {
        if (Globals::verbose)
        {
            std::cout << qPrintable(QString("Subscribing to '%1'\n").arg(subscribeTopic));

            if (incrementTopicPerPublish && topicBase.contains("%1"))
                std::cout << qPrintable(QString("Publishing to '%1' (and increasing number per publish)\n").arg(publishTopic));
            else
                std::cout << qPrintable(QString("Publishing to '%1'\n").arg(publishTopic));
        }
        client->subscribe(subscribeTopic, this->qos);
        publishTimer.start();
    }
    else
    {
        if (Globals::verbose)
            std::cout << qPrintable(QString("Subscribing to '%1'\n").arg(subscribeTopic));
        client->subscribe(subscribeTopic);
    }
}

void OneClient::onDisconnect()
{
    _connected = false;
    counters.disconnect++;

    if (Globals::verbose)
    {
        QString msg = QString("Client %1 disconnected\n").arg(this->client_id);
        std::cout << msg.toLatin1().toStdString().data();
    }
}

void OneClient::onClientError(const QMQTT::ClientError error)
{
    counters.error++;

    // TODO: arg, doesn't qmqtt have a better way for this?
    QString errStr = QString("unknown error");
    if (error == QMQTT::SocketConnectionRefusedError)
        errStr = "Connection refused";
    if (error == QMQTT::SocketRemoteHostClosedError)
        errStr = "Remote host closed";
    if (error == QMQTT::SocketHostNotFoundError)
        errStr = "Remote host not found";
    if (error == QMQTT::MqttBadUserNameOrPasswordError)
        errStr = "MQTT bad user or password";
    if (error == QMQTT::MqttNotAuthorizedError)
        errStr = "MQTT not authorized";
    if (error == QMQTT::SocketResourceError)
        errStr = "Socket resource error. Is your OS limiting you? Ulimit, etc?";
    if (error == QMQTT::SocketSslInternalError)
        errStr = "Socket SSL internal error.";
    if (error == QMQTT::SocketTimeoutError)
        errStr = "Socket timeout";

    if (Globals::verbose)
    {
        QString msg = QString("Client %1 error code: %2 (%3). Initiated delayed reconnect.\n").arg(this->client_id).arg(error).arg(errStr);
        std::cerr << msg.toLatin1().toStdString().data();
    }

    if (regenRandomUsername)
    {
        const QString newUsername = QString(this->usernameBase).arg(GetRandomString());
        client->setUsername(newUsername);
    }

    if (regenRandomPassword)
    {
        const QString newPassword = QString(this->passwordBase).arg(GetRandomString());
        client->setPassword(newPassword.toLatin1());
    }

    this->reconnectTimer.start();

}

void OneClient::onPublishTimerTimeout()
{
    // https://github.com/emqx/qmqtt/issues/230
    if (!_connected)
        return;

    for (int i = 0; i < burstSize; i++)
    {
        if (incrementTopicPerPublish)
        {
            const int nr = ClientNumberPool::getClientNr();
            publishTopic = QString(topicBase).arg(nr);
        }

        QString payload = payloadBase.arg(counters.publish);
        QMQTT::Message msg(0, publishTopic, payload.toUtf8(), this->qos);
        client->publish(msg);
        counters.publish++;
    }
}

void OneClient::onReceived(const QMQTT::Message &message)
{
    Q_UNUSED(message)
    counters.received++;
}
