/*
    WPN-XM Server Control Panel

    WPN-XM SCP is a tool to manage Nginx, PHP and MariaDb daemons under windows.

    It's a fork of Easy WEMP originally written by Yann Le Moigne and (c) 2010.
    WPN-XM SCP is written by Jens-Andre Koch and (c) 2011 - onwards.

    This file is part of WPN-XM Serverpack for Windows.

    WPN-XM SCP is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    WPN-XM SCP is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with WPN-XM SCP. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef HOST_H
#define HOST_H

#include <QList>
#include <QString>

class Host
{
public:
    static QList<Host*> GetHosts();
    static void SetHosts(QList<Host*> listHosts);

    explicit Host();
    explicit Host(QString strName, QString strAddress);

    QString name();
    void setName(QString strName);

    QString address();
    void setAddress(QString strAddress);

    //bool isEnable();
    //void setEnable(bool bEnable);

    bool operator==(const Host &host) const;

private:
    static QString getHostFile();

    //bool m_bIsEnable;
    QString m_strName;
    QString m_strAddress;
};

#endif // HOST_H
