#pragma once
/* BEGIN wsclient.h  */
/* ----------------- */
#include <functional>
#include <memory>
#include <string>

namespace WSClient {
    struct Session : std::enable_shared_from_this<Session> {
        Session();
        ~Session(); // needs to be defined in the .cpp file for pimpl idiom

        void run(std::string host, std::string port);
        void close();
        void send(std::string data);

        using Handler = std::function<void(std::string)>;
        void setHandler(Handler);

      private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
} // namespace WSClient

/* END wsclient.h    */
/* ----------------- */
