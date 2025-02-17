#include "wsclient.h"

namespace WSClient {
    struct Session::Impl {
        // Implementation details
    };
    Session::Session()  = default;
    Session::~Session() = default;

    void Session::run(std::string /*host*/, std::string /*port*/) {
        // Connect to the websocket server
    }

    void Session::close() {
        // Close the websocket connection
    }

    void Session::send(std::string /*data*/) {
        // Send data to the websocket server
    }

    void Session::setHandler(Handler) {
        // Set message handler
    }
} // namespace WSClient
