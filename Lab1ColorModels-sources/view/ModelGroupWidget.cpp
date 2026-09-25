#include "ModelGroupWidget.h"

#include <cmath>

#include <QGridLayout>
#include <QLabel>

ModelGroupWidget::ModelGroupWidget(const QString& title, const std::vector<ComponentSpec>& specs,
                                   QWidget* parent)
    : QGroupBox(title, parent) {
    auto* grid = new QGridLayout(this);
    grid->setVerticalSpacing(10);

    int row = 0;
    for (const ComponentSpec& spec : specs) {
        auto* label = new QLabel(QString::fromUtf8(spec.label), this);
        label->setMinimumWidth(16);

        auto* slider = new QSlider(Qt::Horizontal, this);
        slider->setRange(spec.min, spec.max);

        auto* spin = new QSpinBox(this);
        spin->setRange(spec.min, spec.max);
        spin->setSingleStep(1);
        spin->setAlignment(Qt::AlignRight);
        spin->setMinimumWidth(64);

        const int index = row;
        connect(slider, &QSlider::valueChanged, this, [this, index](int value) {
            if (updating_) {
                return;
            }
            updating_ = true;
            spins_[index]->setValue(value);
            updating_ = false;
            emitCurrentValues();
        });
        connect(spin, qOverload<int>(&QSpinBox::valueChanged), this,
                [this, index](int value) {
                    if (updating_) {
                        return;
                    }
                    updating_ = true;
                    sliders_[index]->setValue(value);
                    updating_ = false;
                    emitCurrentValues();
                });

        grid->addWidget(label, row, 0);
        grid->addWidget(slider, row, 1);
        grid->addWidget(spin, row, 2);
        sliders_.push_back(slider);
        spins_.push_back(spin);
        ++row;
    }

    grid->setColumnStretch(1, 1);
}

void ModelGroupWidget::setValues(const std::array<double, 3>& values) {
    // Точные координаты округляются только для отображения; внутреннее
    // состояние ViewModel остаётся с полной точностью.
    updating_ = true;
    for (int i = 0; i < 3; ++i) {
        const int shown = static_cast<int>(std::lround(values[i]));
        spins_[i]->setValue(shown);
        sliders_[i]->setValue(shown);
    }
    updating_ = false;
}

void ModelGroupWidget::emitCurrentValues() {
    emit valuesChanged(spins_[0]->value(), spins_[1]->value(), spins_[2]->value());
}
