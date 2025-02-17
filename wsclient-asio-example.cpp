#include "wsclient.h"

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/json.hpp>
#include <deque>
#include <iostream>
#include <memory>
#include <string>

// #define BOOST_THREAD_USE_LIB // SEHE: not using Boost Thread, so this is not needed?

namespace beast     = boost::beast;     // from <boost/beast.hpp>
namespace http      = beast::http;      // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net       = boost::asio;      // from <boost/asio.hpp>
namespace ssl       = boost::asio::ssl; // from <boost/asio/ssl.hpp>
namespace json      = boost::json;
using tcp           = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>
using namespace std::chrono_literals;

#define TRACE() std::cerr << __FUNCTION__ << ":" << __LINE__ << std::endl
namespace WSClient {
    // Report a failure
    static void fail(beast::error_code ec, char const* what) { std::cerr << what << ": " << ec.message() << "\n"; }

    // Implementation details completely hidden from the public interface
    struct Session::Impl {
        std::weak_ptr<Impl>   owner_; // aliasing shared_from_this to share owner lifetime
        std::shared_ptr<Impl> shared_from_this() { return owner_.lock(); }

        struct Outgoing {
            std::string text;
            bool        is_close = false;
        };

        net::thread_pool     ioc_{1};
        ssl::context         ssl_ctx_{ssl::context::tlsv12_client};
        tcp::resolver        resolver_{ioc_};
        beast::flat_buffer   buffer_;
        std::string          host_;
        std::deque<Outgoing> queue_;
        std::atomic_bool     ready_{false};
        Handler              on_message_;

        websocket::stream<ssl::stream<beast::tcp_stream>> ws_{ioc_, ssl_ctx_};

      public:
        explicit Impl(std::shared_ptr<void> owner) : owner_(std::shared_ptr<Impl>(owner, this)) {
            TRACE();
            // ws_.binary(true);
            ws_.text(true);
            ws_.auto_fragment(false);
            ws_.write_buffer_bytes(8192);
        }

        ~Impl() {
            std::cout << "~Impl()" << std::endl;
            close(false); // close eagerly
            ioc_.join();
        }

        void run(std::string host, std::string port) {
            TRACE();
            host_ = host;
            resolver_.async_resolve(host, port,
                                    beast::bind_front_handler(&Impl::on_resolve, shared_from_this()));
        }

        void send(Outgoing msg, bool eager = false) {
            TRACE();
            net::post(ws_.get_executor(),
                      beast::bind_front_handler(&Impl::do_send, shared_from_this(), std::move(msg), eager));
        }

        void close(bool eager = false) {
            TRACE();

            bool constexpr is_close = true;
            send(Outgoing{eager ? "bailing out" : "bye", is_close}, eager);
        }

        // convenience overloads
        void send(std::string message) { send(Outgoing{std::move(message)}, false); }
        void send(json::value const& jv) { send(serialize(jv)); }

      private:
        // queues writes (including close), eagerly or lazily
        void do_send(Outgoing msg, bool eager) {
            TRACE();
            std::cout << __FUNCTION__ << "(" << (eager ? "eager" : "lazy") << "): " << msg.text << std::endl;
            if (eager)
                queue_.push_front(std::move(msg));
            else
                queue_.push_back(std::move(msg));

            if (ready_ && queue_.size() == 1)
                do_send_loop(); // Start the write process
        }

        void do_send_loop() { // handles writes, including close
            TRACE();
            if (queue_.empty())
                return;

            auto const& msg      = queue_.front().text; // SEHE: or use c++17 structured bindings
            auto const  is_close = queue_.front().is_close;

            if (is_close) {
                std::cout << "closing: " << msg << std::endl;
                ws_.async_close({websocket::close_code::going_away, msg},
                                [this, self = shared_from_this()](beast::error_code ec) {
                                    if (ec)
                                        return fail(ec, "close");

                                    ws_.next_layer().async_shutdown(
                                        [self](beast::error_code ec) { return fail(ec, "tls shutdown"); });
                                });
            } else {
                std::cout << "writing: " << msg << std::endl;
                // Perform async write, using on_writen as the callback function
                ws_.async_write( //
                    net::buffer(msg),
                    [this, is_close, self = shared_from_this()] //
                    (beast::error_code ec, std::size_t /*bytes_transferred*/) {
                        if (ec)
                            return fail(ec, "write");

                        // Only pop if no error
                        // --------------------
                        // This implies retry on error, but more importantly avoids
                        // UB when the queue is cleared (e.g. for close)
                        queue_.pop_front();

                        // If there are more messages to send, send the next one
                        if (!is_close)
                            do_send_loop();
                    });
            }
        }

        void on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
            TRACE();
            if (ec)
                return fail(ec, "resolve");

            beast::get_lowest_layer(ws_).expires_after(30s);
            beast::get_lowest_layer(ws_).async_connect(
                results, beast::bind_front_handler(&Impl::on_connect, shared_from_this()));
        }

        void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep) {
            TRACE();
            if (ec)
                return fail(ec, "connect");

            if (!SSL_set_tlsext_host_name(ws_.next_layer().native_handle(), host_.c_str())) {
                ec = beast::error_code(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category());
                return fail(ec, "connect");
            }

            host_ += ':' + std::to_string(ep.port());
            ws_.next_layer().async_handshake(
                ssl::stream_base::client,
                beast::bind_front_handler(&Impl::on_ssl_handshake, shared_from_this()));
        }

        void on_ssl_handshake(beast::error_code ec) {
            TRACE();
            if (ec)
                return fail(ec, "ssl_handshake");

            ws_.set_option(websocket::stream_base::decorator([](websocket::request_type& req) {
                req.set(http::field::user_agent,
                        std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-async-ssl");
            }));

            ws_.async_handshake(host_, "/",
                                beast::bind_front_handler(&Impl::on_handshake, shared_from_this()));
        }

        void on_handshake(beast::error_code ec) {
            TRACE();
            if (ec)
                return fail(ec, "handshake");

            beast::get_lowest_layer(ws_).expires_never();
            ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));

            ready_ = true;
            do_send_loop(); // in case messages were already queued
            do_read_loop(); // read loop to detect disconnects and keep the connection alive
        }

        void do_read_loop() {
            TRACE();
            ws_.async_read(
                buffer_,
                [this, self = shared_from_this()](beast::error_code ec, std::size_t bytes_transferred) {
                    TRACE();
                    if (ec)
                        return fail(ec, "read");

                    std::string msg = beast::buffers_to_string(buffer_.data()).substr(0, bytes_transferred);
                    buffer_.consume(bytes_transferred);
                    do_read_loop();

                    if (on_message_)
                        on_message_(std::move(msg));
                });
        }

        // instantiation helper
        friend struct Session;
        static Impl* get(std::shared_ptr<void> const&);
    };

    // instantiation helper; implementations need to be defined after Session::Impl is a complete type
    Session::Impl* Session::Impl::get(std::shared_ptr<void> const& v) {
        auto s = std::static_pointer_cast<Session>(v);
        if (s->impl_ == nullptr)
            s->impl_ = std::make_unique<Impl>(v);
        return s->impl_.get();
    }
} // namespace WSClient

///////////////////////////////////////////////////////////////////////////
// Public interface, trivially hides the implementation details
namespace WSClient {
    Session::Session()  = default;
    Session::~Session() = default;

    void Session::run(std::string host, std::string port) { //
        Impl::get(shared_from_this())->run(std::move(host), std::move(port));
    }
    void Session::close() { //
        Impl::get(shared_from_this())->close();
    }
    void Session::send(std::string data) { //
        Impl::get(shared_from_this())->send(std::move(data));
    }
    void Session::setHandler(Handler on_message) { //
        Impl::get(shared_from_this())->on_message_ = std::move(on_message);
    }
} // namespace WSClient
// End of public interface
///////////////////////////////////////////////////////////////////////////
