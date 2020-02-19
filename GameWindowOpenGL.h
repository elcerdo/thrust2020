#include <QOpenGLWindow>
#include <QOpenGLExtraFunctions>

#include <vector>
#include <tuple>

class GameWindowOpenGL : public QOpenGLWindow, private QOpenGLExtraFunctions
{
    Q_OBJECT
    public:
        using BoolCallback = std::function<void(bool)>;
        using BoolState = std::tuple<std::string, bool, BoolCallback>;
        using BoolStates = std::vector<BoolState>;

        using FloatCallback = std::function<void(float)>;
        using FloatState = std::tuple<std::string, float, float, float, FloatCallback>;
        using FloatStates = std::vector<FloatState>;

        GameWindowOpenGL(QWindow* parent = nullptr);
        void setAnimated(const bool value);
        void addSlider(const std::string& label, const float& min, const float& max, const float& value, const FloatCallback& callback);
        void addCheckbox(const std::string& label, const bool& value, const BoolCallback& callback);

    protected:
        void initializeGL() override;
        void paintGL() override;

    private:
        //bool show_test_window = true;
        //bool show_another_window = false;
        float clear_color[4] = { 1, 0, 0, 1 };
        bool is_animated = false;

        BoolStates bool_states;
        FloatStates float_states;
};

