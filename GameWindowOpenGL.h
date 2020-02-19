#include <QOpenGLWindow>
#include <QOpenGLExtraFunctions>

class GameWindowOpenGL : public QOpenGLWindow, private QOpenGLExtraFunctions
{
    Q_OBJECT
    public:
        GameWindowOpenGL(QWindow* parent = nullptr);
        void setAnimated(const bool value);

    protected:
        void initializeGL() override;
        void paintGL() override;

    private:
        //bool show_test_window = true;
        //bool show_another_window = false;
        float clear_color[4] = { 1, 0, 0, 1 };
        bool is_animated = false;
};

