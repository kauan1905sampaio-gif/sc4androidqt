#include <QDateTime>
#include "EditorWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
#include <QComboBox>
#include <QGroupBox>
#include <QMessageBox>

EditorWidget::EditorWidget(QWidget *parent) : QWidget(parent) {
    auto *root = new QVBoxLayout(this);

    // Export selector
    auto *expRow = new QHBoxLayout;
    expRow->addWidget(new QLabel("Export:"));
    m_exportCombo = new QComboBox;
    m_exportCombo->setMinimumWidth(200);
    expRow->addWidget(m_exportCombo, 1);
    m_cloneBtn = new QPushButton("Clone");
    expRow->addWidget(m_cloneBtn);
    root->addLayout(expRow);

    // Frames
    auto *frGrp = new QGroupBox("Frames");
    auto *frLayout = new QVBoxLayout(frGrp);
    m_frameList = new QListWidget;
    m_frameList->setMaximumHeight(100);
    frLayout->addWidget(m_frameList);
    auto *frBtns = new QHBoxLayout;
    m_addFrameBtn = new QPushButton("+ Add Frame");
    m_rmFrameBtn  = new QPushButton("− Remove");
    frBtns->addWidget(m_addFrameBtn);
    frBtns->addWidget(m_rmFrameBtn);
    frLayout->addLayout(frBtns);
    root->addWidget(frGrp);

    // Elements
    auto *elGrp = new QGroupBox("Elements");
    auto *elLayout = new QVBoxLayout(elGrp);
    m_elementList = new QListWidget;
    elLayout->addWidget(m_elementList);
    root->addWidget(elGrp, 1);

    // Position controls
    auto *posGrp = new QGroupBox("Position Edit");
    auto *posLayout = new QHBoxLayout(posGrp);
    posLayout->addWidget(new QLabel("Δ X:"));
    m_dxSpin = new QSpinBox; m_dxSpin->setRange(-9999, 9999);
    posLayout->addWidget(m_dxSpin);
    posLayout->addWidget(new QLabel("Δ Y:"));
    m_dySpin = new QSpinBox; m_dySpin->setRange(-9999, 9999);
    posLayout->addWidget(m_dySpin);
    m_moveBtn = new QPushButton("Apply");
    m_moveBtn->setStyleSheet("QPushButton{background:#FFD54F;color:#0E0E14;font-weight:bold;}");
    posLayout->addWidget(m_moveBtn);
    root->addWidget(posGrp);

    m_statusLabel = new QLabel("No file loaded");
    root->addWidget(m_statusLabel);

    // Connections
    connect(m_exportCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
        if (m_exportCombo->count() == 0) return;
        m_currentExportId = m_exportCombo->currentData().toInt();
        m_selectedFrame   = 0;
        m_selectedElement = -1;
        refreshFrames();
    });
    connect(m_frameList, &QListWidget::currentRowChanged,
            this, &EditorWidget::onFrameChanged);
    connect(m_elementList, &QListWidget::currentRowChanged,
            this, [this](int row){ m_selectedElement = row; });
    connect(m_moveBtn,     &QPushButton::clicked, this, &EditorWidget::onMoveElement);
    connect(m_addFrameBtn, &QPushButton::clicked, this, &EditorWidget::onAddFrame);
    connect(m_rmFrameBtn,  &QPushButton::clicked, this, &EditorWidget::onRemoveFrame);
    connect(m_cloneBtn,    &QPushButton::clicked, this, &EditorWidget::onCloneExport);
}

void EditorWidget::setFile(const ScFile &file) {
    m_file = file;
    m_exportCombo->blockSignals(true);
    m_exportCombo->clear();
    for (const auto &exp : m_file.exports)
        m_exportCombo->addItem(exp.name, exp.id);
    m_exportCombo->blockSignals(false);
    if (!m_file.exports.isEmpty()) {
        m_currentExportId = m_file.exports[0].id;
        refreshFrames();
    }
}

void EditorWidget::selectExport(int exportId) {
    for (int i = 0; i < m_exportCombo->count(); i++) {
        if (m_exportCombo->itemData(i).toInt() == exportId) {
            m_exportCombo->setCurrentIndex(i);
            break;
        }
    }
}

Movieclip *EditorWidget::currentMovieclip() {
    for (const auto &exp : m_file.exports) {
        if (exp.id != m_currentExportId) continue;
        for (auto &mc : m_file.movieclips)
            if (mc.id == exp.movieclipId) return &mc;
    }
    return nullptr;
}

void EditorWidget::refreshFrames() {
    m_frameList->clear();
    auto *mc = currentMovieclip();
    if (!mc) return;
    for (int i = 0; i < mc->frames.size(); i++)
        m_frameList->addItem(mc->frames[i].name.isEmpty()
                             ? QString("Frame %1").arg(i) : mc->frames[i].name);
    if (m_selectedFrame < mc->frames.size())
        m_frameList->setCurrentRow(m_selectedFrame);
    refreshElements();
}

void EditorWidget::onFrameChanged(int idx) {
    m_selectedFrame   = idx;
    m_selectedElement = -1;
    refreshElements();
}

void EditorWidget::refreshElements() {
    m_elementList->clear();
    auto *mc = currentMovieclip();
    if (!mc || m_selectedFrame >= mc->frames.size()) return;
    const auto &frame = mc->frames[m_selectedFrame];
    for (const auto &el : frame.elements) {
        QString label = QString("Shape #%1  |  Matrix: %2  |  CS: %3")
            .arg(el.shapeId).arg(el.matrixIndex).arg(el.colorSpaceIndex);
        m_elementList->addItem(label);
    }
}

void EditorWidget::onMoveElement() {
    if (m_selectedElement < 0) { m_statusLabel->setText("Select an element first"); return; }
    auto *mc = currentMovieclip();
    if (!mc || m_selectedFrame >= mc->frames.size()) return;
    auto &frame = mc->frames[m_selectedFrame];
    if (m_selectedElement >= frame.elements.size()) return;

    auto &el = frame.elements[m_selectedElement];
    float dx = m_dxSpin->value(), dy = m_dySpin->value();

    // Check if existing matrix is identity+translation — accumulate
    if (el.matrixIndex < m_file.matrices.size()) {
        auto &m = m_file.matrices[el.matrixIndex];
        if (qAbs(m.a-1)<0.01f && qAbs(m.b)<0.01f &&
            qAbs(m.c)<0.01f && qAbs(m.d-1)<0.01f) {
            m.tx += dx; m.ty += dy;
            m_statusLabel->setText(QString("Moved to tx=%1 ty=%2").arg(m.tx).arg(m.ty));
            return;
        }
    }
    // Create new translation matrix
    Matrix nm; nm.id=m_file.matrices.size(); nm.a=1;nm.b=0;nm.c=0;nm.d=1;nm.tx=dx;nm.ty=dy;
    m_file.matrices.append(nm);
    el.matrixIndex = nm.id;
    m_statusLabel->setText(QString("Applied tx=%1 ty=%2").arg(dx).arg(dy));
    refreshElements();
}

void EditorWidget::onAddFrame() {
    auto *mc = currentMovieclip();
    if (!mc) return;
    MovieclipFrame fr;
    fr.name = QString("frame_%1").arg(QDateTime::currentMSecsSinceEpoch());
    mc->frames.append(fr);
    refreshFrames();
}

void EditorWidget::onRemoveFrame() {
    auto *mc = currentMovieclip();
    if (!mc || mc->frames.isEmpty()) return;
    mc->frames.removeAt(m_selectedFrame);
    m_selectedFrame = qMax(0, m_selectedFrame-1);
    refreshFrames();
}

void EditorWidget::onCloneExport() {
    if (m_currentExportId < 0) return;
    Export *srcExp = nullptr;
    for (auto &e : m_file.exports) if (e.id == m_currentExportId) { srcExp=&e; break; }
    Movieclip *srcMc = currentMovieclip();
    if (!srcExp || !srcMc) return;

    int newMcId = 0, newExpId = 0;
    for (const auto &m : m_file.movieclips) newMcId  = qMax(newMcId, m.id+1);
    for (const auto &e : m_file.exports)    newExpId = qMax(newExpId, e.id+1);

    Movieclip newMc = *srcMc; newMc.id = newMcId;
    Export    newEx = *srcExp; newEx.id = newExpId;
    newEx.name += "_clone"; newEx.movieclipId = newMcId;

    m_file.movieclips.append(newMc);
    m_file.exports.append(newEx);
    m_exportCombo->addItem(newEx.name, newEx.id);
    QMessageBox::information(this, "Cloned", "Created: " + newEx.name);
}
