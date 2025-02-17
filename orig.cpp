// Simple Qt application with two buttons to test the websocket with:
#include <QApplication>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

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

    virtual ~TestClient() override {}

  public slots:
    void onButton_ConnectClicked();
    void onButton_SendDataClicked();
    QPushButton* button_SendData;
};

void TestClient::onButton_ConnectClicked() {
    // TODO
}

void TestClient::onButton_SendDataClicked() {
    // TODO
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    TestClient   client;
    client.show();
    return app.exec();
}
