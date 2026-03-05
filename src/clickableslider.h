#ifndef CLICKABLESLIDER_H
#define CLICKABLESLIDER_H

#include <QSlider>
#include <QStyleOptionSlider>
#include <QMouseEvent>

class ClickableSlider : public QSlider
{
    Q_OBJECT
public:
    explicit ClickableSlider(QWidget *parent = nullptr);
    explicit ClickableSlider(Qt::Orientation orientation, QWidget *parent = nullptr);
    
protected:
    void mousePressEvent(QMouseEvent *event) override;
    
private:
    int pixelPosToRangeValue(const QPoint &pos);
};

#endif // CLICKABLESLIDER_H