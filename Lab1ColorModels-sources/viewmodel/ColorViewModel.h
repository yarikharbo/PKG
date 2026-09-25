#ifndef COLORVIEWMODEL_H
#define COLORVIEWMODEL_H

// ViewModel layer: Qt Core glue between the View (widgets) and the Model
// (pure math). Knows nothing about QWidget; the View only calls its slots
// and reads its state via the stateChanged() signal.

#include <array>

#include <QObject>
#include <QString>

#include "ColorConverter.h"

class ColorViewModel : public QObject {
    Q_OBJECT

public:
    explicit ColorViewModel(QObject* parent = nullptr);

    const colormodel::Rgb& rgb() const { return rgb_; }
    const colormodel::Xyz& xyz() const { return xyz_; }
    const colormodel::Lab& lab() const { return lab_; }

    // True when the last conversion to RGB required clamping (color outside
    // the sRGB gamut), e.g. after entering an XYZ/Lab color that sRGB cannot
    // display exactly.
    bool clipped() const { return clipped_; }
    const std::array<bool, 3>& clippedChannels() const { return clippedChannels_; }

    // Human-readable, non-intrusive warning; empty when nothing was clipped.
    QString warningText() const;

public slots:
    void setRgb(int r, int g, int b);
    void setXyz(double x, double y, double z);
    void setLab(double l, double a, double b);

signals:
    void stateChanged();

private:
    void commit(const colormodel::Rgb& rgb, const colormodel::Xyz& xyz,
                const colormodel::Lab& lab, bool clipped,
                const std::array<bool, 3>& clippedChannels);

    colormodel::Rgb rgb_{255, 0, 0};
    colormodel::Xyz xyz_;
    colormodel::Lab lab_;
    bool clipped_ = false;
    std::array<bool, 3> clippedChannels_{};
};

#endif  // COLORVIEWMODEL_H
