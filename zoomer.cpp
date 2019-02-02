#include "Zoomer.h"
#include "qwt_scale_widget.h"
#include "qwt_plot.h"


Zoomer::Zoomer(QwtPlotCanvas *canvas,QRectF rect, bool bDir):
    QwtPlotZoomer(canvas)
{
    m_bDir = bDir;
    m_rect = rect.normalized();
}
void Zoomer::rescale()
{
    QwtScaleWidget *scaleWidget = plot()->axisWidget(yAxis());
    QwtScaleDraw *sd = scaleWidget->scaleDraw();

    int minExtent = 0;
    if ( zoomRectIndex() > 0 )
    {
        // When scrolling in vertical direction
        // the plot is jumping in horizontal direction
        // because of the different widths of the labels
        // So we better use a fixed extent.

        minExtent = sd->spacing() + 1;//+ sd->majTickLength() 
        minExtent += sd->labelSize(
            scaleWidget->font(), c_rangeMax).width();
    }

    sd->setMinimumExtent(minExtent);

    QwtPlotZoomer::rescale();
}
void Zoomer::updateRange(QRectF rect)
{
    m_rect = rect.normalized();
}
void Zoomer::zoom( const QRectF &rect )
{
    QRectF zoomRect = rect.normalized();
    QwtPlot *plot = QwtPlotZoomer::plot();
    if ( !plot )
        return;

    QPointF lt = zoomRect.topLeft();
    QPointF rb = zoomRect.bottomRight();
    if(m_bDir)
    {
	lt.setY(m_rect.top());
	rb.setY(m_rect.bottom());

	if(lt.x()-m_rect.left()<1e-10)
		lt.setX(m_rect.left());
	if(rb.x()-m_rect.left()<1e-10)
		rb.setX(m_rect.left());

	if(lt.x()-m_rect.right()>1e-10)
		lt.setX(m_rect.right());
	if(rb.x()-m_rect.right()>1e-10)
		rb.setX(m_rect.right());

	QRectF zoom(lt,rb);
	QwtPlotZoomer::zoom(zoom);
    }
    else
    {
	lt.setX(m_rect.left());
	rb.setX(m_rect.right());

	if(lt.y()-m_rect.top()<1e-10)
		lt.setY(m_rect.top());
	if(rb.y()-m_rect.top()<1e-10)
		rb.setY(m_rect.top());

	if(lt.y()-m_rect.bottom()>1e-10)
		lt.setY(m_rect.bottom());
	if(rb.y()-m_rect.bottom()>1e-10)
		rb.setY(m_rect.bottom());

	QRectF zoom(lt,rb);
	QwtPlotZoomer::zoom(zoom);
    }

    plot->replot();
}
void Zoomer::zoom( int up )
{
	QwtPlotZoomer::zoom(up);
	QwtPlot *plot = QwtPlotZoomer::plot();
	plot->replot();
}

void Zoomer::setRect(QRectF rect)
{
    m_rect=rect;
}
