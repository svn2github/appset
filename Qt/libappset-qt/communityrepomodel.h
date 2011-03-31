#ifndef COMMUNITYREPOMODEL_H
#define COMMUNITYREPOMODEL_H

#include <QAbstractTableModel>
#include <QModelIndex>
#include <QString>

#include "asqtnixengine.h"

#include <vector>

class CommunityRepoModel : public QAbstractTableModel
{
    Q_OBJECT

    QString pattern;

    AS::QTNIXEngine *engine;
    std::vector<AS::Package*>* pkgs;

public:
    explicit CommunityRepoModel(QObject *parent = 0, AS::QTNIXEngine *engine=0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const ;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;



signals:
    void pkgInfoRetrieved(AS::Package *pkg);

public slots:
    void selectionChangedSlot(const QModelIndex & newSelection, const QModelIndex & oldSelection);

    void setPattern(QString pattern);

};

#endif // COMMUNITYREPOMODEL_H
