#include "ViewerWidget.h"
#include <QTabWidget>
#include <QListWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>

ViewerWidget::ViewerWidget(QWidget *parent) : QWidget(parent) {
    m_tabs       = new QTabWidget(this);
    m_exportList = new QListWidget;
    m_shapeList  = new QListWidget;
    m_texList    = new QListWidget;
    m_matList    = new QListWidget;
    m_infoLabel  = new QLabel;
    m_infoLabel->setWordWrap(true);
    m_infoLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    m_tabs->addTab(m_exportList, "Exports");
    m_tabs->addTab(m_shapeList,  "Shapes");
    m_tabs->addTab(m_texList,    "Textures");
    m_tabs->addTab(m_matList,    "Matrices");
    m_tabs->addTab(m_infoLabel,  "Info");

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(m_tabs);

    // Double-click export → open editor
    connect(m_exportList, &QListWidget::itemDoubleClicked,
            this, [this](QListWidgetItem *item) {
        emit exportSelected(item->data(Qt::UserRole).toInt());
    });
}

void ViewerWidget::setFile(const ScFile &file) {
    m_file = file;
    buildExportsTab();
    buildShapesTab();
    buildTexturesTab();
    buildMatricesTab();
    buildInfoTab();
}

void ViewerWidget::buildExportsTab() {
    m_exportList->clear();
    for (const auto &exp : m_file.exports) {
        auto *item = new QListWidgetItem(
            QString("▶  %1  [ID:%2  MC:%3]").arg(exp.name).arg(exp.id).arg(exp.movieclipId));
        item->setData(Qt::UserRole, exp.id);
        item->setForeground(QColor(0xFF,0xD5,0x4F));
        m_exportList->addItem(item);
    }
}
void ViewerWidget::buildShapesTab() {
    m_shapeList->clear();
    for (const auto &s : m_file.shapes)
        m_shapeList->addItem(QString("Shape #%1  (%2 chunks)").arg(s.id).arg(s.chunks.size()));
}
void ViewerWidget::buildTexturesTab() {
    m_texList->clear();
    for (const auto &t : m_file.textures)
        m_texList->addItem(QString("Texture #%1  %2×%3").arg(t.index).arg(t.width).arg(t.height));
}
void ViewerWidget::buildMatricesTab() {
    m_matList->clear();
    for (const auto &m : m_file.matrices)
        m_matList->addItem(QString("Matrix #%1  [%.2f,%.2f,%.2f,%.2f]  tx=%.1f ty=%.1f")
            .arg(m.id).arg(m.a).arg(m.b).arg(m.c).arg(m.d).arg(m.tx).arg(m.ty));
}
void ViewerWidget::buildInfoTab() {
    QString comprStr;
    switch (m_file.compression) {
        case Compression::LZMA: comprStr="LZMA"; break;
        case Compression::ZSTD: comprStr="ZSTD"; break;
        case Compression::LZHAM:comprStr="LZHAM";break;
        default:                comprStr="None"; break;
    }
    m_infoLabel->setText(QString(
        "<b>Version:</b> %1<br>"
        "<b>Compression:</b> %2<br>"
        "<b>Exports:</b> %3<br>"
        "<b>Movieclips:</b> %4<br>"
        "<b>Shapes:</b> %5<br>"
        "<b>Textures:</b> %6<br>"
        "<b>Matrices:</b> %7<br>"
        "<b>ColorSpaces:</b> %8<br>"
        "<b>TextFields:</b> %9<br>"
        "<b>Path:</b> %10"
    ).arg(m_file.version).arg(comprStr)
     .arg(m_file.exports.size()).arg(m_file.movieclips.size())
     .arg(m_file.shapes.size()).arg(m_file.textures.size())
     .arg(m_file.matrices.size()).arg(m_file.colorSpaces.size())
     .arg(m_file.textFields.size()).arg(m_file.path));
}
