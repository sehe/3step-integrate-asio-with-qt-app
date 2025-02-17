/* BEGIN wsclient.h  */
/* ----------------- */
#include <string>

namespace WSClient {
    struct Session {
        void run(std::string host, std::string port);
        void close();
        void send(std::string data);
    };
} // namespace WSClient

/* END wsclient.h    */
/* ----------------- */

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
