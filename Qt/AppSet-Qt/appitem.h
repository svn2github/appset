/*
Copyright 2011 Simone Tobia

This file is part of AppSet.

AppSet is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

AppSet is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with AppSet; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#ifndef APPITEM_H
#define APPITEM_H

#include <QObject>

class AppItem : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString lversion READ lversion WRITE setLVersion NOTIFY lversionChanged)
    Q_PROPERTY(QString rversion READ rversion WRITE setRVersion NOTIFY rversionChanged)
    Q_PROPERTY(QString appUrl READ appUrl WRITE setAppUrl NOTIFY appUrlChanged)
    Q_PROPERTY(QString status READ status WRITE setStatus NOTIFY statusChanged)
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged)
    Q_PROPERTY(int dsize READ dsize WRITE setDSize NOTIFY dSizeChanged)
    Q_PROPERTY(QString repo READ repo WRITE setRepo NOTIFY repoChanged)
    Q_PROPERTY(QString deps READ deps WRITE setDeps NOTIFY depsChanged)

    Q_PROPERTY(int i READ i WRITE setI NOTIFY iChanged)

    Q_PROPERTY(QString lversionstr READ lversionstr WRITE setLVersionStr NOTIFY lversionstrChanged)
    Q_PROPERTY(QString rversionstr READ rversionstr WRITE setRVersionStr NOTIFY rversionstrChanged)
    Q_PROPERTY(QString dsizestr READ dsizestr WRITE setDSizeStr NOTIFY dsizestrChanged)
    Q_PROPERTY(QString repostr READ repostr WRITE setRepoStr NOTIFY repostrChanged)
    Q_PROPERTY(QString closestr READ closestr WRITE setCloseStr NOTIFY closestrChanged)
    Q_PROPERTY(QString installstr READ installstr WRITE setInstallStr NOTIFY installstrChanged)
    Q_PROPERTY(QString updatestr READ updatestr WRITE setUpdateStr NOTIFY updatestrChanged)
    Q_PROPERTY(QString removestr READ removestr WRITE setRemoveStr NOTIFY removestrChanged)
public:
    explicit AppItem(QObject *parent = 0);
    //AppItem(const QString &name, const QString &lversion, const QString &rversion, const QString &appUrl, const QString &picture, const QString &description);

    QString name() const;
    void setName(const QString &name);

    QString lversion() const;
    void setLVersion(const QString &lversion);

    QString rversion() const;
    void setRVersion(const QString &rversion);

    QString appUrl() const;
    void setAppUrl(const QString &appUrl);

    QString status() const;
    void setStatus(const QString &status);

    QString description() const;
    void setDescription(const QString &description);

    int dsize() const;
    void setDSize(const int &dsize);

    QString repo() const;
    void setRepo(const QString &repo);

    QString deps() const;
    void setDeps(const QString &deps);

    int i() const;
    void setI(const int &i);

    QString lversionstr() const{return m_lversionstr;}
    void setLVersionStr(const QString &lvstr){this->m_lversionstr=lvstr;emit lversionstrChanged();};

    QString rversionstr() const{return m_rversionstr;}
    void setRVersionStr(const QString &rvstr){this->m_rversionstr=rvstr;emit rversionstrChanged();};

    QString dsizestr() const{return m_dsizestr;}
    void setDSizeStr(const QString &dsizestr){this->m_dsizestr=dsizestr;emit dsizestrChanged();};

    QString repostr() const{return m_repostr;}
    void setRepoStr(const QString &repostr){this->m_repostr=repostr;emit repostrChanged();};

    QString closestr() const{return m_closestr;}
    void setCloseStr(const QString &closestr){this->m_closestr=closestr;emit closestrChanged();};

    QString installstr() const{return m_installstr;}
    void setInstallStr(const QString &installstr){this->m_installstr=installstr;emit installstrChanged();};

    QString updatestr() const{return m_updatestr;}
    void setUpdateStr(const QString &updatestr){this->m_updatestr=updatestr;emit updatestrChanged();};

    QString removestr() const{return m_removestr;}
    void setRemoveStr(const QString &removestr){this->m_removestr=removestr;emit removestrChanged();};
signals:
    void nameChanged();
    void lversionChanged();
    void rversionChanged();
    void appUrlChanged();
    void statusChanged();
    void descriptionChanged();
    void iChanged();
    void dSizeChanged();
    void repoChanged();
    void depsChanged();

    void lversionstrChanged();
    void rversionstrChanged();
    void dsizestrChanged();
    void repostrChanged();
    void closestrChanged();
    void installstrChanged();
    void updatestrChanged();
    void removestrChanged();
private:
    QString m_name;
    QString m_lversion;
    QString m_rversion;
    QString m_appUrl;
    QString m_status;
    QString m_description;
    int m_index;
    int m_dsize;
    QString m_repo;
    QString m_deps;

    QString m_lversionstr;
    QString m_rversionstr;
    QString m_dsizestr;
    QString m_repostr;
    QString m_closestr;
    QString m_installstr;
    QString m_updatestr;
    QString m_removestr;
};

#endif // APPITEM_H
