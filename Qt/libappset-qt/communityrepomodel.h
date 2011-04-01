#ifndef COMMUNITYREPOMODEL_H
#define COMMUNITYREPOMODEL_H

#include <QAbstractTableModel>
#include <QModelIndex>
#include <QString>
#include <QStringList>

#include "asqtnixengine.h"

#include <vector>

class CommunityRepoModel : public QAbstractTableModel
{
    Q_OBJECT

    QString pattern;

    AS::QTNIXEngine *engine;
    std::vector<AS::Package*> *pkgs;
    std::list<AS::Package*> *ipkgs;

    QStringList headers;

    void mergeInstalled();
    bool merged;
public:
    explicit CommunityRepoModel(QStringList headers, QObject *parent = 0, AS::QTNIXEngine *engine=0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const ;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;


signals:
    void pkgInfoRetrieved(AS::Package *pkg);
    void dataUpdated();

public slots:
    void selectionChangedSlot(const QModelIndex & newSelection, const QModelIndex & oldSelection);

    void setPattern(QString pattern);

    void setInstalledPackages(std::list<AS::Package*> *pkgs);

};

#endif // COMMUNITYREPOMODEL_H
