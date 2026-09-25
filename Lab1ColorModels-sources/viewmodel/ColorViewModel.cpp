#include "ColorViewModel.h"

#include <algorithm>
#include <utility>

#include <QStringList>

using colormodel::Lab;
using colormodel::LabToXyz;
using colormodel::Rgb;
using colormodel::RgbFromXyz;
using colormodel::RgbToXyz;
using colormodel::Xyz;
using colormodel::XyzToLab;
using colormodel::XyzToRgbDetailed;

ColorViewModel::ColorViewModel(QObject* parent) : QObject(parent) {
    const Xyz initial = RgbToXyz(rgb_);
    xyz_ = initial;
    lab_ = XyzToLab(initial);
}

void ColorViewModel::setRgb(int r, int g, int b) {
    const Rgb rgb{std::clamp(r, 0, 255), std::clamp(g, 0, 255), std::clamp(b, 0, 255)};
    const Xyz xyz = RgbToXyz(rgb);
    // RGB input is always inside the sRGB gamut by construction.
    commit(rgb, xyz, XyzToLab(xyz), false, {false, false, false});
}

void ColorViewModel::setXyz(double x, double y, double z) {
    const Xyz xyz{x, y, z};
    const RgbFromXyz res = XyzToRgbDetailed(xyz);
    commit(res.value, xyz, XyzToLab(xyz), res.clipped, res.clippedChannels);
}

void ColorViewModel::setLab(double l, double a, double b) {
    const Lab lab{std::clamp(l, 0.0, 100.0), a, b};
    const Xyz xyz = LabToXyz(lab);
    const RgbFromXyz res = XyzToRgbDetailed(xyz);
    commit(res.value, xyz, lab, res.clipped, res.clippedChannels);
}

QString ColorViewModel::warningText() const {
    if (!clipped_) {
        return {};
    }
    QStringList channels;
    if (clippedChannels_[0]) {
        channels << QStringLiteral("R");
    }
    if (clippedChannels_[1]) {
        channels << QStringLiteral("G");
    }
    if (clippedChannels_[2]) {
        channels << QStringLiteral("B");
    }
    return QStringLiteral("Цвет вне диапазона sRGB: выполнено обрезание компонент (")
        + channels.join(QStringLiteral(", ")) + QStringLiteral(").");
}

void ColorViewModel::commit(const Rgb& rgb, const Xyz& xyz, const Lab& lab, bool clipped,
                            const std::array<bool, 3>& clippedChannels) {
    rgb_ = rgb;
    xyz_ = xyz;
    lab_ = lab;
    clipped_ = clipped;
    clippedChannels_ = clippedChannels;
    emit stateChanged();
}
