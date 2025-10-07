#include "MyController.h"
#include <QDebug>

static QString keyOf(QObject* s) {
    return s->property("peerKey").toString();
}

QString MyController::makeKey(const QString &ip, int port){
    return ip + ":" + QString::number(port);
}

MyController::MyController(QObject *parent) : QObject(parent)
{
    // Nada aquí; peers se crean bajo demanda
}

// ===== Compatibilidad con tus pantallas de habitación =====
void MyController::conectar(const QString &ip, int puerto)
{
    setActivePeer(ip, puerto);
    emit estadoConexion(QStringLiteral("⌛ Conectando a %1:%2...").arg(ip).arg(puerto));
}

void MyController::enviarComando(const QString &comando)
{
    Peer* p = activePeer();
    if (!p || !p->sock || p->sock->state()!=QTcpSocket::ConnectedState) {
        emit mensajeRecibido(QStringLiteral("❌ No hay conexión."));
        return;
    }
    QByteArray line = comando.toUtf8();
    if (line.isEmpty() || line.back()!='\n') line.append('\n');
    p->sock->write(line);
    p->sock->flush();
}

void MyController::desconectar()
{
    Peer* p = activePeer();
    if (p && p->sock) p->sock->disconnectFromHost();
}

// ===== Nuevo: multi-peer =====
MyController::Peer* MyController::ensurePeer(const QString &ip, int port)
{
    const QString key = makeKey(ip, port);
    if (peers.contains(key)) return peers.value(key);

    Peer* p = new Peer();
    p->sock = new QTcpSocket(this);
    p->sock->setProperty("peerKey", key);
    peers.insert(key, p);
    wirePeerSignals(key, p);
    p->sock->connectToHost(ip, port);
    return p;
}

void MyController::addPeer(const QString &ip, int port)
{
    ensurePeer(ip, port);
    emit estadoConexionFrom(makeKey(ip,port), QStringLiteral("⌛ Conectando..."));
}

void MyController::setActivePeer(const QString &ip, int port)
{
    Peer* p = ensurePeer(ip, port);
    m_activeKey = makeKey(ip, port);
    emit activeKeyChanged(m_activeKey);

    // Estado "global" para compatibilidad con tu UI actual
    setConnected(p->sock->state()==QTcpSocket::ConnectedState);
}

void MyController::sendTo(const QString &ip, int port, const QString &cmd)
{
    Peer* p = ensurePeer(ip, port);
    if (!p || !p->sock) return;
    QByteArray line = cmd.toUtf8();
    if (line.isEmpty() || line.back()!='\n') line.append('\n');
    if (p->sock->state()==QTcpSocket::ConnectedState) {
        p->sock->write(line);
        p->sock->flush();
    }
}

void MyController::sendToAll(const QString &cmd)
{
    QByteArray line = cmd.toUtf8();
    if (line.isEmpty() || line.back()!='n') line.append('\n');
    for (auto it = peers.begin(); it != peers.end(); ++it) {
        Peer* p = it.value();
        if (p && p->sock && p->sock->state()==QTcpSocket::ConnectedState) {
            p->sock->write(line);
            p->sock->flush();
        }
    }
}

MyController::Peer* MyController::activePeer() const
{
    return peers.value(m_activeKey, nullptr);
}

void MyController::wirePeerSignals(const QString &key, Peer* p)
{
    connect(p->sock, &QTcpSocket::readyRead, this, &MyController::onSocketReadyRead);
    connect(p->sock, &QTcpSocket::connected,  this, &MyController::onSocketConnected);
    connect(p->sock, &QTcpSocket::disconnected, this, &MyController::onSocketDisconnected);
    connect(p->sock, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &MyController::onSocketError);
}

void MyController::onSocketReadyRead()
{
    QTcpSocket* s = qobject_cast<QTcpSocket*>(sender());
    if (!s) return;
    const QString key = keyOf(s);
    Peer* p = peers.value(key, nullptr);
    if (!p) return;

    p->buffer += QString::fromUtf8(s->readAll());
    int idx;
    while ((idx = p->buffer.indexOf('\n')) != -1) {
        QString line = p->buffer.left(idx);
        p->buffer.remove(0, idx+1);
        line = line.trimmed();

        // Señal “nueva” con origen:
        emit mensajeRecibidoFrom(key, line);
        // Compatibilidad con tu UI actual (sin origen):
        emit mensajeRecibido(line);
    }
}

void MyController::onSocketConnected()
{
    QTcpSocket* s = qobject_cast<QTcpSocket*>(sender());
    if (!s) return;
    const QString key = keyOf(s);

    if (key == m_activeKey) {
        setConnected(true);
        emit estadoConexion(QStringLiteral("✅ Conectado"));
    }
    emit estadoConexionFrom(key, QStringLiteral("✅ Conectado"));
}

void MyController::onSocketDisconnected()
{
    QTcpSocket* s = qobject_cast<QTcpSocket*>(sender());
    if (!s) return;
    const QString key = keyOf(s);

    if (key == m_activeKey) {
        setConnected(false);
        emit estadoConexion(QStringLiteral("⚠️ Desconectado"));
    }
    emit estadoConexionFrom(key, QStringLiteral("⚠️ Desconectado"));
}

void MyController::onSocketError(QAbstractSocket::SocketError)
{
    QTcpSocket* s = qobject_cast<QTcpSocket*>(sender());
    if (!s) return;
    const QString key = keyOf(s);

    if (key == m_activeKey) {
        setConnected(false);
        emit estadoConexion(QStringLiteral("❌ Error: %1").arg(s->errorString()));
    }
    emit estadoConexionFrom(key, QStringLiteral("❌ Error: %1").arg(s->errorString()));
}

void MyController::setConnected(bool c)
{
    if (m_connected == c) return;
    m_connected = c;
    emit connectedChanged(m_connected);
}
