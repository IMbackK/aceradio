#include "clickableslider.h"
#include <QStyle>

ClickableSlider::ClickableSlider(QWidget *parent)
    : QSlider(Qt::Orientation::Horizontal, parent)
{
}

ClickableSlider::ClickableSlider(Qt::Orientation orientation, QWidget *parent)
    : QSlider(orientation, parent)
{
}

void ClickableSlider::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        int val = pixelPosToRangeValue(event->pos());
        
        // Block signals temporarily to avoid infinite recursion
        blockSignals(true);
        setValue(val);
        blockSignals(false);
        
        // Emit both valueChanged and sliderMoved signals for compatibility
        emit valueChanged(val);
        emit sliderMoved(val);
    } else {
        // Call base class implementation for other buttons
        QSlider::mousePressEvent(event);
    }
}

int ClickableSlider::pixelPosToRangeValue(const QPoint &pos)
{
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    
    QRect gr = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this);
    QRect sr = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);

    int sliderLength;
    int sliderMin;
    int sliderMax;
    
    if (orientation() == Qt::Horizontal) {
        sliderLength = sr.width();
        sliderMin = gr.x();
        sliderMax = gr.right() - sliderLength + 1;
    } else {
        sliderLength = sr.height();
        sliderMin = gr.y();
        sliderMax = gr.bottom() - sliderLength + 1;
    }
    
    QPoint pr = pos - sr.center() + sr.topLeft();
    int p = orientation() == Qt::Horizontal ? pr.x() : pr.y();
    
    return QStyle::sliderValueFromPosition(minimum(), maximum(), p - sliderMin,
                                           sliderMax - sliderMin, opt.upsideDown);
}
