#pragma once

#include <QtGui>

class RasterWindow : public QWindow
{
    Q_OBJECT
    public:
        explicit RasterWindow(QWindow *parent = nullptr);
        virtual void render(QPainter& painter) = 0;

    public slots:
        void renderLater();
        void renderNow();

    protected:
        bool event(QEvent *event) override;
        void resizeEvent(QResizeEvent *event) override;
        void exposeEvent(QExposeEvent *event) override;
        void timerEvent(QTimerEvent *event) override;

    private:
        QBackingStore *m_backingStore;
};

