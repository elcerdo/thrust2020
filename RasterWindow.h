#pragma once

#include <QtGui>

class RasterWindow : public QWindow
{
    Q_OBJECT
    public:
        explicit RasterWindow(QWindow *parent = nullptr);
        virtual void render(QPainter& painter) = 0;
        void setAnimated(const bool is_animated_);
        bool isAnimated() const;

    public slots:
        void renderLater();
        void renderNow();

    protected:
        bool event(QEvent *event) override;
        void resizeEvent(QResizeEvent *event) override;
        void exposeEvent(QExposeEvent *event) override;

    private:
        QBackingStore *m_backingStore;
        bool is_animated;
};

