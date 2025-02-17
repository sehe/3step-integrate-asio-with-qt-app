#include "wsclient.h"

// this could be in message-processing.h/cpp to keep this file clean
#include <boost/json.hpp>
#include <iostream>
static void process_message(std::string const& msg) {
    std::cerr << __FUNCTION__ << ":" << 3 << std::endl;

    boost::system::error_code ec;
    boost::json::value        jsonValue = boost::json::parse(msg, ec);

    if (ec) {
        std::cerr << __FUNCTION__ << ": Error parsing JSON: " << ec.message() << std::endl;
        std::cerr << " -- " << msg << std::endl;
    }

    std::cout << __FUNCTION__ << ": Received JSON: " << jsonValue << std::endl;
}

// Simple Qt application with two buttons to test the websocket with:
//
#include <QApplication>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <memory>

class TestClient : public QWidget {
    //Q_OBJECT

  public:
    TestClient() {
        auto layout = new QVBoxLayout(this);
        auto button = new QPushButton("Connect", this);
        layout->addWidget(button);
        connect(button, &QPushButton::clicked, this, &TestClient::onButton_ConnectClicked);

        button = new QPushButton("Send Data", this);
        button->setEnabled(false);
        layout->addWidget(button);
        connect(button, &QPushButton::clicked, this, &TestClient::onButton_SendDataClicked);

        button_SendData = button;
    }

    virtual ~TestClient() override {
        if (auto session = current_.lock())
            session->close();
        current_.reset();
    }

  public slots:
    void onButton_ConnectClicked();
    void onButton_SendDataClicked();
    QPushButton* button_SendData;

  private:
    std::weak_ptr<WSClient::Session> current_;
};

void TestClient::onButton_ConnectClicked() {
    if (auto session = current_.lock())
        session->close();

    // Establish a new connection to the server
    auto session = std::make_shared<WSClient::Session>();
    session->setHandler(process_message);
    session->run("0.0.0.0", "8080");
    session->send(R"({"name": "ConnectionButton", "status": "Clicked"})");
    current_ = session;

    if (button_SendData)
        button_SendData->setEnabled(true);
}

void TestClient::onButton_SendDataClicked() {
    if (auto session = current_.lock()) {
        session->send(R"({"name": "SendDataButton", "status": "Clicked"})");
    } else if (button_SendData) {
        button_SendData->setEnabled(false);
    }
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    TestClient   client;
    client.show();
    return app.exec();
}
