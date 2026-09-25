#ifndef MODELGROUPWIDGET_H
#define MODELGROUPWIDGET_H

// View helper: a QGroupBox with three color components, each editable in
// two ways -- with a slider (smooth change) and with an integer numeric
// field. Used for the RGB, XYZ and Lab groups alike. Input and display are
// integers only; the ViewModel keeps the exact (unrounded) coordinates.

#include <array>
#include <vector>

#include <QGroupBox>
#include <QSlider>
#include <QSpinBox>

class ModelGroupWidget : public QGroupBox {
    Q_OBJECT

public:
    struct ComponentSpec {
        const char* label;
        int min;
        int max;
    };

    ModelGroupWidget(const QString& title, const std::vector<ComponentSpec>& specs,
                     QWidget* parent = nullptr);

    // Pushes (exact, unrounded) values into the widgets without emitting
    // valuesChanged(). The fields display them rounded to integers.
    void setValues(const std::array<double, 3>& values);

signals:
    // Emitted once per user edit (slider drag step, spin change).
    void valuesChanged(double c0, double c1, double c2);

private:
    void emitCurrentValues();

    std::vector<QSlider*> sliders_;
    std::vector<QSpinBox*> spins_;
    bool updating_ = false;  // guards against feedback loops
};

#endif  // MODELGROUPWIDGET_H
