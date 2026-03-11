#include "protocol/MessageDispatcher.h"
#include "network/TcpConnection.h"
#include "utils/Logger.h"

namespace gomoku {

MessageDispatcher::MessageDispatcher() {
}

MessageDispatcher::~MessageDispatcher() {
}

bool MessageDispatcher::dispatchMessage(const std::shared_ptr<TcpConnection>& conn, 
                                         const ProtobufMessagePtr& message) {
    if (!message) {
        LOG_ERROR("MessageDispatcher::dispatchMessage - null message");
        return false;
    }
    
    const std::string& typeName = message->GetTypeName();
    
    auto it = callbacks_.find(typeName);
    if (it != callbacks_.end()) {
        try {
            it->second(conn, message);
            return true;
        } catch (const std::exception& e) {
            LOG_ERROR("MessageDispatcher::dispatchMessage exception: " + std::string(e.what()));
            return false;
        }
    } else {
        LOG_WARN("MessageDispatcher::dispatchMessage - no callback for message type: " + typeName);
        return false;
    }
}

MessageCallback MessageDispatcher::getMessageCallback(const std::string& typeName) const {
    auto it = callbacks_.find(typeName);
    if (it != callbacks_.end()) {
        return it->second;
    }
    return nullptr;
}

} // namespace gomoku