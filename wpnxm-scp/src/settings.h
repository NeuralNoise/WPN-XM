/*
    WPN-XM Server Control Panel

    WPN-XM SCP is a tool to manage Nginx, PHP and MariaDB daemons under windows.
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

#ifndef SETTINGS_H
#define SETTINGS_H

// local includes
#include "settingsTable.h"

/// Implements the application settings repository.
/*!
    This class stores the application settings.
    Defaults can be saved and restored from session to session.
*/
class Settings : public SettingsTable
{
    Q_OBJECT

public:

    /// Default constructor
    Settings(QObject *parent = 0);

    /// Destructor
    ~Settings();

    /// Save defaults to the file.
    void saveSettings() const;

    /// Reads defaults from the file.
    void readSettings();

private:

    /// Returns defaults file's full path and name.
    QString file() const;
};

#endif // SETTINGS_H
