#pragma once

#include <QWidget>
#include <vector>

class HistogramWidget : public QWidget {
    Q_OBJECT

public:
    explicit HistogramWidget(QWidget* parent = nullptr);
    void setHistogram(const std::vector<int>& hist, const QString& title);
    void setChannelHistograms(const std::vector<int>& r, const std::vector<int>& g, const std::vector<int>& b,
                              const QString& title);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    std::vector<int> hist_;
    std::vector<int> ch_[3];
    bool channels_ = false;
    QString title_;
};
