#ifndef SDIO_WINDOW_MOTION_H
#define SDIO_WINDOW_MOTION_H
#include <QObject>
#include <QImage>
#include <QTimer>
#include <QElapsedTimer>
#include <QRect>
class QWidget;
class MotionSnapshot;
// One bounded raster capture, then at most five software-composited frames.
// The live interface changes geometry only once per state transition.
class WindowMotion : public QObject {
public:
    explicit WindowMotion(QWidget *window);
    ~WindowMotion();
    void minimize();
    void toggleMaximized();
    void restoredFromMinimized();
    bool busy()const{return m_busy;}
protected:
    bool eventFilter(QObject *watched,QEvent *event) Q_DECL_OVERRIDE;
private:
    QImage capture();
    QRect taskbarTarget()const;
    void begin(const QImage &image,const QRect &from,const QRect &to,bool minimizing);
    void tick();
    void finish();
    QWidget *m_window;
    MotionSnapshot *m_proxy;
    QTimer m_timer;
    QElapsedTimer m_clock;
    QRect m_from,m_to;
    bool m_busy=false,m_minimizing=false;
    int m_frames=0;
};
#endif
