#include <QAbstractNativeEventFilter>
#include <QDebug>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QtWebEngineQuick/QtWebEngineQuick>

#include "AttentionController.h"

#ifdef Q_OS_WIN
#define NOMINMAX
#include <windows.h>
#include <wtsapi32.h>

class SessionLockFilter final : public QAbstractNativeEventFilter
{
public:
    SessionLockFilter(QWindow *window, AttentionController *controller)
        : m_window(reinterpret_cast<HWND>(window->winId())), m_controller(controller)
    {
        if (!WTSRegisterSessionNotification(m_window, NOTIFY_FOR_THIS_SESSION))
            qWarning() << "Could not register for Windows lock notifications.";
        if (!RegisterHotKey(m_window, 1, MOD_CONTROL | MOD_ALT, VK_ESCAPE))
            qWarning() << "Could not register Ctrl+Alt+Escape.";
        if (!RegisterHotKey(m_window, 2, MOD_CONTROL | MOD_ALT, 'Q'))
            qWarning() << "Could not register Ctrl+Alt+Q.";
    }

    ~SessionLockFilter() override
    {
        UnregisterHotKey(m_window, 1);
        UnregisterHotKey(m_window, 2);
        WTSUnRegisterSessionNotification(m_window);
    }

    bool nativeEventFilter(const QByteArray &, void *message, qintptr *result) override
    {
        const auto *nativeMessage = static_cast<MSG *>(message);
        if (nativeMessage->message == WM_HOTKEY) {
            if (nativeMessage->wParam == 1)
                m_controller->dismissPrivacyScreen();
            else if (nativeMessage->wParam == 2)
                QCoreApplication::quit();
            else
                return false;
            *result = 0;
            return true;
        }
        if (nativeMessage->message != WM_WTSSESSION_CHANGE)
            return false;

        if (nativeMessage->wParam == WTS_SESSION_LOCK)
            m_controller->setSessionLocked(true);
        else if (nativeMessage->wParam == WTS_SESSION_UNLOCK)
            m_controller->setSessionLocked(false);
        else
            return false;

        *result = 0;
        return false;
    }

private:
    HWND m_window;
    AttentionController *m_controller;
};
#endif

int main(int argc, char *argv[])
{
    QtWebEngineQuick::initialize();
    QGuiApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("Eye Attention Qt"));

    AttentionController controller;
    controller.setObjectName(QStringLiteral("attentionController"));
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("attentionController"), &controller);
    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return 1;

#ifdef Q_OS_WIN
    auto *mainWindow = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
    if (!mainWindow)
        return 1;
    SessionLockFilter sessionLockFilter(mainWindow, &controller);
    app.installNativeEventFilter(&sessionLockFilter);
#endif

    return app.exec();
}
