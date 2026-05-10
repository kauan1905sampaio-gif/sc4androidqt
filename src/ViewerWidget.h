#pragma once
#include <QWidget>
#include "ScFile.h"
class QTabWidget; class QListWidget; class QLabel;

class ViewerWidget : public QWidget {
    Q_OBJECT
public:
    explicit ViewerWidget(QWidget *parent = nullptr);
    void setFile(const ScFile &file);
signals:
    void exportSelected(int exportId);
private:
    void buildExportsTab();
    void buildShapesTab();
    void buildTexturesTab();
    void buildMatricesTab();
    void buildInfoTab();
    QTabWidget  *m_tabs;
    QListWidget *m_exportList, *m_shapeList, *m_texList, *m_matList;
    QLabel      *m_infoLabel;
    ScFile m_file;
};
