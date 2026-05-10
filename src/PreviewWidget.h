#pragma once
#include <QWidget>
#include <QTimer>
#include "ScFile.h"
class QPushButton; class QLabel; class QSlider;

class PreviewWidget : public QWidget {
    Q_OBJECT
public:
    explicit PreviewWidget(QWidget *parent = nullptr);
    void setFile(const ScFile &file);
    void selectExport(int exportId);
protected:
    void paintEvent(QPaintEvent *) override;
private slots:
    void onPlayPause();
    void onTick();
    void onPrev();
    void onNext();
private:
    void buildTextures();
    QImage pixelsToImage(const ScTexture &tex);
    ScFile m_file;
    int    m_exportId = -1;
    int    m_frame    = 0;
    bool   m_playing  = false;
    QVector<QImage> m_texImages;
    QTimer  *m_timer;
    QPushButton *m_playBtn, *m_prevBtn, *m_nextBtn;
    QLabel      *m_frameLabel;
    QSlider     *m_fpsSlider;
};
