#include "HistogramWidget.h"

#include <QPainter>
#include <QPainterPath>

#include <algorithm>

HistogramWidget::HistogramWidget(QWidget* parent)
    : QWidget(parent) {
    setMinimumHeight(120);
}

void HistogramWidget::setHistogram(const std::vector<int>& hist, const QString& title) {
    hist_ = hist;
    channels_ = false;
    title_ = title;
    update();
}

void HistogramWidget::setChannelHistograms(const std::vector<int>& r, const std::vector<int>& g,
                                           const std::vector<int>& b, const QString& title) {
    ch_[0] = r;
    ch_[1] = g;
    ch_[2] = b;
    channels_ = true;
    title_ = title;
    update();
}

void HistogramWidget::clear() {
    hist_.clear();
    ch_[0].clear();
    ch_[1].clear();
    ch_[2].clear();
    title_.clear();
    update();
}

void HistogramWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), palette().window());

    const int top = title_.isEmpty() ? 4 : 20;
    const int bottom = 16;
    const QRectF area(4, top, width() - 8, height() - top - bottom);

    p.setPen(palette().color(QPalette::WindowText));
    if (!title_.isEmpty()) p.drawText(rect().adjusted(4, 2, -4, -4), Qt::AlignTop | Qt::AlignHCenter, title_);

    p.setPen(palette().color(QPalette::Mid));
    p.drawRect(area);
    for (int q = 1; q < 4; ++q) {
        const double x = area.left() + area.width() * q / 4.0;
        p.drawLine(QPointF(x, area.top()), QPointF(x, area.bottom()));
    }

    if (!channels_ && hist_.size() == 256) {
        const long long maxv = *std::max_element(hist_.begin(), hist_.end());
        if (maxv > 0) {
            QPainterPath path;
            path.moveTo(area.left(), area.bottom());
            for (int i = 0; i < 256; ++i) {
                const double x = area.left() + area.width() * i / 255.0;
                const double y = area.bottom() - area.height() * hist_[static_cast<size_t>(i)] / static_cast<double>(maxv);
                path.lineTo(x, y);
            }
            path.lineTo(area.right(), area.bottom());
            p.fillPath(path, QColor(70, 130, 180, 160));
        }
    } else if (channels_) {
        long long maxv = 1;
        for (int c = 0; c < 3; ++c)
            for (int v : ch_[c]) maxv = std::max<long long>(maxv, v);
        static const QColor colors[3] = {QColor(200, 60, 60, 170), QColor(60, 170, 60, 170), QColor(60, 60, 200, 170)};
        for (int c = 0; c < 3; ++c) {
            QPainterPath path;
            path.moveTo(area.left(), area.bottom());
            for (int i = 0; i < 256; ++i) {
                const double x = area.left() + area.width() * i / 255.0;
                const double y = area.bottom() - area.height() * ch_[c][static_cast<size_t>(i)] / static_cast<double>(maxv);
                path.lineTo(x, y);
            }
            path.lineTo(area.right(), area.bottom());
            p.fillPath(path, colors[c]);
        }
    }

    p.setPen(palette().color(QPalette::WindowText));
    p.drawText(rect().adjusted(4, 0, -4, -2), Qt::AlignBottom | Qt::AlignLeft, QStringLiteral("0"));
    p.drawText(rect().adjusted(4, 0, -4, -2), Qt::AlignBottom | Qt::AlignRight, QStringLiteral("255"));
}
