#include "messagedispatcher.h"
#include <QDebug>

MessageDispatcher::MessageDispatcher(QObject* parent)
    : QObject(parent)
{
}

MessageDispatcher::~MessageDispatcher()
{
    clear();
}

void MessageDispatcher::dispatch(int messageType, const QByteArray& data)
{
    auto it = handlers_.find(messageType);
    if (it != handlers_.end()) {
        try {
            it->second->handle(data);
        } catch (const std::exception& e) {
            qWarning() << "Exception in message handler:" << e.what();
        }
    } else {
        qWarning() << "No handler registered for message type:" << messageType;
    }
}

void MessageDispatcher::unregisterHandler(int messageType)
{
    handlers_.erase(messageType);
}

void MessageDispatcher::clear()
{
    handlers_.clear();
}