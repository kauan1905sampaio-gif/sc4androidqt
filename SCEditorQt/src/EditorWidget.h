#pragma once
#include <QWidget>
#include "ScFile.h"
class QListWidget; class QLabel; class QSpinBox; class QPushButton; class QComboBox;

class EditorWidget : public QWidget {
    Q_OBJECT
public:
    explicit EditorWidget(QWidget *parent = nullptr);
    void setFile(const ScFile &file);
    void selectExport(int exportId);
private slots:
    void onFrameChanged(int idx);
    void onMoveElement();
    void onAddFrame();
    void onRemoveFrame();
    void onCloneExport();
private:
    void refreshFrames();
    void refreshElements();
    Movieclip *currentMovieclip();
    ScFile m_file;
    int    m_currentExportId = -1;
    int    m_selectedFrame   = 0;
    int    m_selectedElement = -1;

    QComboBox   *m_exportCombo;
    QListWidget *m_frameList;
    QListWidget *m_elementList;
    QSpinBox    *m_dxSpin, *m_dySpin;
    QPushButton *m_moveBtn, *m_addFrameBtn, *m_rmFrameBtn, *m_cloneBtn;
    QLabel      *m_statusLabel;
};
