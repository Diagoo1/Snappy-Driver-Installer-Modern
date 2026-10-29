#ifndef SDIO_SCAN_OVERLAY_H
#define SDIO_SCAN_OVERLAY_H
#include <QWidget>
#include <QPixmap>
class QLabel;
class QTimer;
class QResizeEvent;
class QPaintEvent;

// Opaque live-scan cover with a blurred snapshot and a Windows-style orbit of
// animated dots. The worker must stay off the GUI thread for this to animate.
class ScanOverlay : public QWidget {
public:
    explicit ScanOverlay(QWidget *parent=0);
    bool isActive() const { return m_active; }
    void start(const QString &message,bool reducedMotion=false);
    void startOperation(const QString &title,const QString &message,bool reducedMotion=false);
    void setStage(const QString &message);
    void finish();
protected:
    bool eventFilter(QObject *object,QEvent *event) Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
private:
    void layoutChildren();
    QPixmap m_snapshot;
    QLabel *m_title,*m_stage,*m_caption;
    QTimer *m_timer;
    bool m_active,m_reducedMotion;
    int m_frame;
};
#endif
