#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QTimer>
#include <QKeyEvent>
#include <QDateTime>
#include <QFont>
#include <cstdio>

class HelloWindow : public QWidget {
public:
    HelloWindow() {
        setFixedSize(320, 170);
        setWindowFlags(Qt::FramelessWindowHint);
        setStyleSheet("background-color: black;");

        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(8, 8, 8, 8);
        layout->setSpacing(6);

        m_title = new QLabel("Hello CardputerZero", this);
        QFont tf = m_title->font();
        tf.setPointSize(14);
        tf.setBold(true);
        m_title->setFont(tf);
        m_title->setStyleSheet("color: white;");
        m_title->setAlignment(Qt::AlignCenter);

        m_clock = new QLabel(this);
        QFont cf = m_clock->font();
        cf.setPointSize(10);
        m_clock->setFont(cf);
        m_clock->setStyleSheet("color: #00ff88;");
        m_clock->setAlignment(Qt::AlignCenter);

        m_hint = new QLabel("ESC / Q to exit", this);
        QFont hf = m_hint->font();
        hf.setPointSize(8);
        m_hint->setFont(hf);
        m_hint->setStyleSheet("color: #888;");
        m_hint->setAlignment(Qt::AlignCenter);

        layout->addStretch();
        layout->addWidget(m_title);
        layout->addWidget(m_clock);
        layout->addStretch();
        layout->addWidget(m_hint);

        auto *timer = new QTimer(this);
        QObject::connect(timer, &QTimer::timeout, this, &HelloWindow::tick);
        timer->start(1000);
        tick();
    }

protected:
    void keyPressEvent(QKeyEvent *e) override {
        if (e->key() == Qt::Key_Escape || e->key() == Qt::Key_Q) {
            qApp->quit();
            return;
        }
        QWidget::keyPressEvent(e);
    }

private:
    void tick() {
        m_clock->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    }

    QLabel *m_title;
    QLabel *m_clock;
    QLabel *m_hint;
};

int main(int argc, char **argv) {
    std::fprintf(stderr, "[hello_qt] launching on 320x170 linuxfb\n");
    QApplication app(argc, argv);
    HelloWindow w;
    w.show();
    return app.exec();
}
