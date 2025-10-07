#ifndef MYCONTROLLER_H
#define MYCONTROLLER_H

#include <QObject>
#include <QTcpSocket>
#include <QHash>

class MyController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(QString activeKey READ activeKey NOTIFY activeKeyChanged)
public:
    explicit MyController(QObject *parent = nullptr);

    // ===== Compatibilidad con tus pantallas de habitación =====
    Q_INVOKABLE void conectar(const QString &ip, int puerto);   // activa (y conecta) 1 socket
    Q_INVOKABLE void enviarComando(const QString &comando);     // envía al activo
    Q_INVOKABLE void desconectar();

    // ===== Nuevo: multi-peer =====
    Q_INVOKABLE void addPeer(const QString &ip, int port);      // prepara (y conecta) un peer
    Q_INVOKABLE void setActivePeer(const QString &ip, int port);// el que usan HabX
    Q_INVOKABLE void sendTo(const QString &ip, int port, const QString &cmd);
    Q_INVOKABLE void sendToAll(const QString &cmd);
    Q_INVOKABLE QString activeKey() const { return m_activeKey; }

    bool isConnected() const { return m_connected; }

signals:
    // Igual que antes (para tus pantallas actuales)
    void mensajeRecibido(const QString &linea);
    void estadoConexion(const QString &estado);
    void connectedChanged(bool connected);

    // Opcional: con info de origen (para GlobalControls, si quieres etiquetar)
    void mensajeRecibidoFrom(const QString &key, const QString &linea);
    void estadoConexionFrom(const QString &key, const QString &estado);
    void activeKeyChanged(const QString &key);

private:
    struct Peer {
        QTcpSocket* sock = nullptr;
        QString buffer;
    };

    // Clave "ip:puerto" -> Peer
    QHash<QString, Peer*> peers;
    QString m_activeKey;
    bool m_connected = false;

    // Internos
    static QString makeKey(const QString &ip, int port);
    void wirePeerSignals(const QString &key, Peer* p);
    void setConnected(bool c);
    Peer* ensurePeer(const QString &ip, int port);
    Peer* activePeer() const;

private slots:
    // Slots comunes para todos los peers (usamos lambdas al conectar)
    void onSocketReadyRead();
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError);
};

#endif // MYCONTROLLER_H
