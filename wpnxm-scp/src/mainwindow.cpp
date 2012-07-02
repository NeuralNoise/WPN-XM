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

// Local includes
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "tray.h"
#include "configurationdialog.h"

// Global includes
#include <QApplication>
#include <QMessageBox>
#include <QSharedMemory>
#include <QtGui>
#include <QRegExp>
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // disable Maximize functionality
    setWindowFlags( (windowFlags() | Qt::CustomizeWindowHint) & ~Qt::WindowMaximizeButtonHint);
    setFixedWidth(620);
    setFixedHeight(320);

    // overrides the window title defined in mainwindow.ui
    setWindowTitle(APP_NAME_AND_VERSION);

    checkAlreadyActiveDaemons();

    // inital state of status leds is disabled
    ui->label_Nginx_Status->setEnabled(false);
    ui->label_PHP_Status->setEnabled(false);
    ui->label_MariaDb_Status->setEnabled(false);

    createActions();

    createTrayIcon();

    // fetch version numbers from the daemons and set label text accordingly
    ui->label_Nginx_Version->setText( getNginxVersion() );
    ui->label_PHP_Version->setText( getPHPVersion() );
    ui->label_MariaDb_Version->setText( getMariaVersion() );

    // hardcode ports for v0.3.0
    ui->label_Nginx_Port->setText("80");
    ui->label_PHP_Port->setText("9100");
    ui->label_MariaDb_Port->setText("3306");

    showPushButtonsOnlyForInstalledTools();
    enableToolsPushButtons(false);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete trayIcon;
}

void MainWindow::createTrayIcon()
{
    // The tray icon is an instance of the QSystemTrayIcon class.
    // To check whether a system tray is present on the user's desktop,
    // we call the static QSystemTrayIcon::isSystemTrayAvailable() function.
    if (false == QSystemTrayIcon::isSystemTrayAvailable())
    {
        QMessageBox::critical(0, APP_NAME, tr("You don't have a system tray."));
        //return 1;
    }
    else
    {
        // instantiate and attach the tray icon to the system tray
        trayIcon = new Tray(qApp);

        // the following actions point to SLOTS in the trayIcon object
        // therefore connections must be made, after constructing trayIcon

        // handle clicks on the icon
        connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
                this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));

        // Connect Actions for Status Table - Column Status
        // if process state of a daemon changes, then change the label status in UI::MainWindow too
        connect(trayIcon, SIGNAL(signalSetLabelStatusActive(QString, bool)),
                this, SLOT(setLabelStatusActive(QString, bool)));

        // Connect Action for, if process state of NGINX and PHP changes,
        // then change the disabled/ enabled state of pushButtons too
        connect(trayIcon, SIGNAL(signalEnableToolsPushButtons(bool)),
                this, SLOT(enableToolsPushButtons(bool)));

        // Connect Actions for Status Table - Column Action (Start)
        connect(ui->pushButton_StartNginx, SIGNAL(clicked()), trayIcon, SLOT(startNginx()));
        connect(ui->pushButton_StartPHP, SIGNAL(clicked()), trayIcon, SLOT(startPhp()));
        connect(ui->pushButton_StartMariaDb, SIGNAL(clicked()), trayIcon, SLOT(startMariaDB()));

         // Connect Actions for Status Table - Column Action - Stop
        connect(ui->pushButton_StopNginx, SIGNAL(clicked()), trayIcon, SLOT(stopNginx()));
        connect(ui->pushButton_StopPHP, SIGNAL(clicked()), trayIcon, SLOT(stopPhp()));
        connect(ui->pushButton_StopMariaDb, SIGNAL(clicked()), trayIcon, SLOT(stopMariaDB()));

         // Connect Actions for Status Table - AllDaemons Start, Stop
        connect(ui->pushButton_AllDaemons_Start, SIGNAL(clicked()), trayIcon, SLOT(startAllDaemons()));
        connect(ui->pushButton_AllDaemons_Stop, SIGNAL(clicked()), trayIcon, SLOT(stopAllDaemons()));

        // finally: show the tray icon
        trayIcon->show();
    }
}

void MainWindow::createActions()
 {
     // title bar - minimize
     minimizeAction = new QAction(tr("Mi&nimize"), this);
     connect(minimizeAction, SIGNAL(triggered()), this, SLOT(hide()));

     // title bar - restore
     restoreAction = new QAction(tr("&Restore"), this);
     connect(restoreAction, SIGNAL(triggered()), this, SLOT(showNormal()));

     // title bar - close
     // Note that this action is intercepted by MainWindow::closeEvent()
     // Its modified from "quit" to "close to tray" with a msgbox
     // qApp is global pointer to QApplication
     quitAction = new QAction(tr("&Quit"), this);
     connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));

     // PushButtons:: Website, Mailinglist, ReportBug, Donate
     connect(ui->pushButton_Website, SIGNAL(clicked()), this, SLOT(goToWebsite()));
     connect(ui->pushButton_GoogleGroup, SIGNAL(clicked()), this, SLOT(goToGoogleGroup()));
     connect(ui->pushButton_ReportBug, SIGNAL(clicked()), this, SLOT(goToReportIssue()));
     connect(ui->pushButton_Donate, SIGNAL(clicked()), this, SLOT(goToDonate()));

     // PushButtons: Configuration, Help, About, Close
     connect(ui->pushButton_Configuration, SIGNAL(clicked()), this, SLOT(openConfigurationDialog()));
     connect(ui->pushButton_Help, SIGNAL(clicked()), this, SLOT(openHelpDialog()));
     connect(ui->pushButton_About, SIGNAL(clicked()), this, SLOT(openAboutDialog()));

     // @todo the following action is not intercepted by the closeEvent()
     // connect(ui->pushButton_Close, SIGNAL(clicked()), qApp, SLOT(quit()));
     // workaround is to not quit, but hide the window
     connect(ui->pushButton_Close, SIGNAL(clicked()), this, SLOT(hide()));

     // Actions - Tools
     connect(ui->pushButton_tools_phpinfo, SIGNAL(clicked()), this, SLOT(openToolPHPInfo()));
     connect(ui->pushButton_tools_phpmyadmin, SIGNAL(clicked()), this, SLOT(openToolPHPMyAdmin()));
     connect(ui->pushButton_tools_webgrind, SIGNAL(clicked()), this, SLOT(openToolWebgrind()));
     connect(ui->pushButton_tools_adminer, SIGNAL(clicked()), this, SLOT(openToolAdminer()));

     // Actions - Open Projects Folder
     connect(ui->pushButton_OpenProjects_browser, SIGNAL(clicked()), this, SLOT(openProjectFolderInBrowser()));
     connect(ui->pushButton_OpenProjects_Explorer, SIGNAL(clicked()), this, SLOT(openProjectFolderInExplorer()));

     // Actions - Status Table (Config)
     connect(ui->pushButton_ConfigureNginx, SIGNAL(clicked()), this, SLOT(openConfigurationDialogNginx()));
     connect(ui->pushButton_ConfigurePHP, SIGNAL(clicked()), this, SLOT(openConfigurationDialogPHP()));
     connect(ui->pushButton_ConfigureMariaDB, SIGNAL(clicked()), this, SLOT(openConfigurationDialogMariaDB()));

     // Actions - Status Table (Logs)
     connect(ui->pushButton_ShowLog_NginxAccess, SIGNAL(clicked()), this, SLOT(openLogNginxAccess()));
     connect(ui->pushButton_ShowLog_NginxError, SIGNAL(clicked()), this, SLOT(openLogNginxError()));
     connect(ui->pushButton_ShowLog_PHP, SIGNAL(clicked()), this, SLOT(openLogPHP()));
     connect(ui->pushButton_ShowLog_MariaDB, SIGNAL(clicked()), this, SLOT(openLogMariaDB()));
 }

void MainWindow::changeEvent(QEvent *event)
{
    switch (event->type())
    {
        //case QEvent::LanguageChange:
        //    this->ui->retranslateUi(this);
        //    break;
        case QEvent::WindowStateChange:
            {
                // minimize to tray (do not minimize to taskbar)
                if (this->windowState() & Qt::WindowMinimized)
                {
                    // @todo provide configuration options to let the user decide on this
                    //if (Preferences::instance().minimizeToTray())
                    //{
                        QTimer::singleShot(0, this, SLOT(hide()));
                    //}
                }

                break;
            }
        default:
            break;
    }

    QMainWindow::changeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (trayIcon->isVisible()) {
        QMessageBox::information(this, APP_NAME,
             tr("The program will keep running in the system tray.<br>"
                "To terminate the program, choose <b>Quit</b> in the context menu of the system tray."));

        // hide mainwindow
        hide();

        // do not propagate the event to the base class
        event->ignore();
    }
}

void MainWindow::setVisible(bool visible)
{
    minimizeAction->setEnabled(visible);
    restoreAction->setEnabled(isMaximized() || !visible);
    QMainWindow::setVisible(visible);
}

void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
        //case QSystemTrayIcon::Trigger:
        //case QSystemTrayIcon::MiddleClick:

        // Double click toggles dialog display
        case QSystemTrayIcon::DoubleClick:
            if( isVisible() )
                // clicking the tray icon, when the main window is shown, hides it
                setVisible(false);
            else {
                // clicking the tray icon, when the main window is hidden, shows the main window
                show();
                setFocus();
                setWindowState( windowState() & ( ~Qt::WindowMinimized | Qt::WindowActive | Qt::WindowMaximized ) );
            }
            break;
        default:
            break;
    }
}

void MainWindow::enableToolsPushButtons(bool enabled)
{
    // get all PushButtons from the Tools GroupBox of MainWindow::UI
    QList<QPushButton *> allPushButtonsButtons = ui->ToolsGroupBox->findChildren<QPushButton *>();

    // set all PushButtons enabled/disabled
    for(int i = 0; i < allPushButtonsButtons.size(); ++i)
    {
        allPushButtonsButtons[i]->setEnabled(enabled);
    }

    // change state of "Open Projects Folder" >> "Browser" button
    ui->OpenProjectFolderGroupBox
      ->findChild<QPushButton*>("pushButton_OpenProjects_browser")
      ->setEnabled(enabled);
}

void MainWindow::showPushButtonsOnlyForInstalledTools()
{
    // get all PushButtons from the Tools GroupBox of MainWindow::UI
    QList<QPushButton *> allPushButtonsButtons = ui->ToolsGroupBox->findChildren<QPushButton *>();

    // set all PushButtons invisible
    for(int i = 0; i < allPushButtonsButtons.size(); ++i)
    {
       allPushButtonsButtons[i]->setVisible(false);
    }

    // if tool directory exists, show pushButton

    if(QDir(getProjectFolder() + "/webinterface").exists())
    {
        ui->pushButton_tools_phpinfo->setVisible(true);
    }

    if(QDir(getProjectFolder() + "/phpmyadmin").exists())
    {
        ui->pushButton_tools_phpmyadmin->setVisible(true);
    }

    if(QDir(getProjectFolder() + "/adminer").exists())
    {
        ui->pushButton_tools_adminer->setVisible(true);
    }

    if(QDir(getProjectFolder() + "/webgrind").exists())
    {
        ui->pushButton_tools_webgrind->setVisible(true);
    }
}

void MainWindow::setLabelStatusActive(QString label, bool enabled)
{
    if(label == "nginx")
    {
        ui->label_Nginx_Status->setEnabled(enabled);
    }

    if(label == "php")
    {
        ui->label_PHP_Status->setEnabled(enabled);
    }

    if(label == "mariadb")
    {
        ui->label_MariaDb_Status->setEnabled(enabled);
    }
}

QString MainWindow::getNginxVersion()
{
    /*QProcess* processNginx;

    processNginx = new QProcess(this);
    process.setProcessChannelMode(QProcess::MergedChannels);
    //processNginx->setWorkingDirectory(cfgNginxDir);
    //processNginx->start("./nginx", QStringList() << "-v");
    processNginx->waitForFinished(-1);

    //QString p_stdout = processNginx->readAllStandardOutput();*/

    // test
    QString p_stdout = "nginx version: nginx/1.2.1";

    //qDebug() << p_stdout;

    return parseVersionNumber(p_stdout);
}

QString MainWindow::getMariaVersion()
{
    /*QProcess* processMaria;
    processMaria = new QProcess(this);
    process.setProcessChannelMode(QProcess::MergedChannels);
    //processMaria->setWorkingDirectory(cfgMariaDir);
    processMaria->start("./mysqld", QStringList() << "-V"); // upper-case V
    processMaria->waitForFinished(-1);

    //QString p_stdout = processMaria->readAllStandardOutput();*/

    // test
    QString p_stdout = "mysql  Ver 15.1 Distrib 5.5.24-MariaDB, for Win32 (x86)";

    //qDebug() << p_stdout;

    return parseVersionNumber(p_stdout.mid(15));
}

QString MainWindow::getPHPVersion()
{
    /*QProcess* processPhp;

    processPhp = new QProcess(this);
    process.setProcessChannelMode(QProcess::MergedChannels);
    //processPhp->setWorkingDirectory(cfgPHPDir);
    //processPhp->start(cfgPHPDir+cfgPHPExec, QStringList() << "-v");
    processPhp->waitForFinished(-1);

    //QString p_stdout = processPhp->readAllStandardOutput();;*/

    // test
    QString p_stdout = "PHP 5.4.3 (cli) (built: Feb 29 2012 19:06:50)";

    //qDebug() << p_stdout;

    return parseVersionNumber(p_stdout);
}

QString MainWindow::parseVersionNumber(QString stringWithVersion)
{
    //qDebug() << stringWithVersion;

    // The RegExp for matching version numbers is (\d+\.)?(\d+\.)?(\d+\.)?(\*|\d+)
    // The following one is escaped:
    QRegExp regex("(\\d+\\.)?(\\d+\\.)?(\\d+\\.)?(\\*|\\d+)");

    // match
    regex.indexIn(stringWithVersion);

    //qDebug() << regex.cap(0);
    QString cap = regex.cap(0);
    return cap;

// Leave this for debugging reasons
//    int pos = 0;
//    while((pos = regex.indexIn(stringWithVersion, pos)) != -1)
//    {
//        qDebug() << "Match at pos " << pos
//                 << " with length " << regex.matchedLength()
//                 << ", captured = " << regex.capturedTexts().at(0).toLatin1().data()
//                 << ".\n";
//        pos += regex.matchedLength();
//    }
}

void MainWindow::goToWebsite()
{
    QDesktopServices::openUrl(QUrl("http://wpn-xm.org/"));
}

void MainWindow::goToGoogleGroup()
{
    QDesktopServices::openUrl(QUrl("http://groups.google.com/group/wpn-xm/"));
}

void MainWindow::goToReportIssue()
{
    QDesktopServices::openUrl(QUrl("https://github.com/jakoch/WPN-XM/issues/"));
}

void MainWindow::goToDonate()
{
    QDesktopServices::openUrl(QUrl("http://wpn-xm.org/#donate"));
}

void MainWindow::openToolPHPInfo()
{
    QDesktopServices::openUrl(QUrl("http://localhost/webinterface/phpinfo.php"));
}

void MainWindow::openToolPHPMyAdmin()
{
    QDesktopServices::openUrl(QUrl("http://localhost/phpmyadmin/"));
}

void MainWindow::openToolWebgrind()
{
    QDesktopServices::openUrl(QUrl("http://localhost/webgrind/"));
}

void MainWindow::openToolAdminer()
{
    QDesktopServices::openUrl(QUrl("http://localhost/adminer/"));
}

void MainWindow::openProjectFolderInBrowser()
{
    // @todo open only, when Nginx and PHP are running...
    QDesktopServices::openUrl(QUrl("http://localhost"));
}

void MainWindow::openProjectFolderInExplorer()
{
    if(QDir(getProjectFolder()).exists())
    {
        // exec explorer with path to projects
        QDesktopServices::openUrl(QUrl("file:///" + getProjectFolder(), QUrl::TolerantMode));
    }
    else
    {
        QMessageBox::warning(this, tr("Warning"), tr("The projects folder was not found."));
    }
}

QString MainWindow::getProjectFolder() const
{
    return QDir::toNativeSeparators(QApplication::applicationDirPath() + "/www");
}

void MainWindow::openConfigurationDialog()
{
    /*ConfigurationDialog cfgDlg;
    cfgDlg.setWindowTitle("Server Control Panel - Configuration");

    cfgDlg.exec();*/
}

void MainWindow::openConfigurationDialogNginx()
{
    // Open Configuration Dialog - Tab for Nginx
}

void MainWindow::openConfigurationDialogPHP()
{
    // Open Configuration Dialog - Tab for PHP
}

void MainWindow::openConfigurationDialogMariaDB()
{
    // Open Configuration Dialog - Tab for MariaDB
}

void MainWindow::openLogNginxAccess()
{
    //qDebug() << qApp->applicationDirPath() + "/logs/access.log";
    QDesktopServices::openUrl(QUrl("file:///" + qApp->applicationDirPath() + "/logs/access.log", QUrl::TolerantMode));
}

void MainWindow::openLogNginxError()
{
    //qDebug() << qApp->applicationDirPath() + "/logs/error.log";
    QDesktopServices::openUrl(QUrl("file:///" + qApp->applicationDirPath() + "/logs/error.log", QUrl::TolerantMode));
}

void MainWindow::openLogPHP()
{
    //qDebug() << qApp->applicationDirPath() + "/logs/php_error.log";
    QDesktopServices::openUrl(QUrl("file:///" + qApp->applicationDirPath() + "/logs/php_error.log", QUrl::TolerantMode));
}

void MainWindow::openLogMariaDB()
{
    //qDebug() << qApp->applicationDirPath() + "/logs/mariadb_error.log";
    QDesktopServices::openUrl(QUrl("file:///" + qApp->applicationDirPath() + "/logs/mariadb_error.log", QUrl::TolerantMode));
}

void MainWindow::openHelpDialog()
{

}

void MainWindow::openAboutDialog()
{
    QMessageBox::about(this, tr("About WPN-XM"),
        tr("<b>WPN-XM Server Control Panel</b><br>"
        "<table><tr><td><img src=\":/cappuccino64\"></img>&nbsp;&nbsp;</td><td>"
        "<table>"
        "<tr><td><b>Website</b></td><td><a href=\"http://wpn-xm.org/\">http://wpn-xm.org/</a><br></td></tr>"
        "<tr><td><b>License</b></td><td>GNU/GPL version 3, or any later version.<br></td></tr>"
        "<tr><td><b>Author(s)</b></td><td>Yann Le Moigne (C) 2010,<br>Jens-Andr� Koch (C) 2011 - onwards.<br></td></tr>"
        "<tr><td><b>Github</b></td><td>WPN-XM is developed on Github.<br><a href=\"https://github.com/jakoch/WPN-XM/\">https://github.com/jakoch/WPN-XM/</a><br></td></tr>"
        "<tr><td><b>Icons</b></td><td>We are using Yusukue Kamiyamane's Fugue Icon Set.<br><a href=\"http://p.yusukekamiyamane.com/\">http://p.yusukekamiyamane.com/</a><br></td></tr>"
        "<tr><td><b>+1?</b></td><td>If you like using WPN-XM, consider supporting it:<br><a href=\"http://wpn-xm.org/donate.html\">http://wpn-xm.org/donate.html</a><br></td></tr>"
        "</td></tr></table></td></tr></table>"
        "<br><br>The program is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.<br>"
        ));
}

void MainWindow::checkAlreadyActiveDaemons()
{
    // Check list of active processes for
    // apache, nginx, mysql, php-cgi, memcached
    // and report if processes are already running.
    // We take a look for these processes to avoid collisions.

    // Provide a modal dialog with checkboxes for all running processes
    // The user might then select the processes to Leave Running or Shutdown.

    // a) fetch processes via tasklist stdout
    QProcess process;
    process.setReadChannel(QProcess::StandardOutput);
    process.setReadChannelMode(QProcess::MergedChannels);
    process.start("cmd", QStringList() << "/c tasklist.exe");
    process.waitForFinished();
    // processList contains the tasklist output
    QByteArray processList = process.readAll();
    //qDebug() << "Read" << processList.length() << "bytes";
    //qDebug() << processList;

    // b) define processes to look for
    QStringList processesToSearch;
    processesToSearch << "nginx"
                      << "apache"
                      << "memcached"
                      << "mysqld"
                      << "php-cgi";

    // c) init a list for found processes
    QStringList processesFoundList;

    // d) foreach processesToSearch take a look in the processList
    for (int i = 0; i < processesToSearch.size(); ++i)
    {
        //qDebug() << "Searching for process: " << processesToSearch.at(i).toLocal8Bit().constData() << endl;

        if(processList.contains( processesToSearch.at(i).toAscii().constData() ))
        {
            // process found
            processesFoundList << processesToSearch.at(i).toAscii().constData();
        }
    }

    qDebug() << "Already running Processes found : " << processesFoundList;

    // only show the "process shutdown" dialog, when there are processes to shutdown
    if(false == processesFoundList.isEmpty())
    {
        QLabel *labelA = new QLabel(tr("The following processes are already running:"));

        QGroupBox *groupBox = new QGroupBox(tr("Running Processes"));

        QCheckBox *checkBox1 = new QCheckBox(tr("&Checkbox 1"));
        QCheckBox *checkBox2 = new QCheckBox(tr("C&heckbox 2"));
        checkBox2->setChecked(true);

        QVBoxLayout *vbox = new QVBoxLayout;
        vbox->addWidget(checkBox1);
        vbox->addWidget(checkBox2);
        //vbox->addStretch(1);
        groupBox->setLayout(vbox);

        QLabel *labelB = new QLabel(tr("Please select the processes you wish to shutdown.<br>"
                                       "Click Yes to shut processes down and continue installation, or click No to proceed.<br>"));

        QPushButton *okShutdownButton = new QPushButton(tr("Shutdown"));
        QPushButton *noShutdownButton = new QPushButton(tr("Continue"));
        okShutdownButton->setDefault(true);

        QDialogButtonBox *buttonBox = new QDialogButtonBox(Qt::Horizontal);
        buttonBox->addButton(okShutdownButton, QDialogButtonBox::ActionRole);
        buttonBox->addButton(noShutdownButton, QDialogButtonBox::ActionRole);

        // e) build dialog to inform user about running processes
        QGridLayout *grid = new QGridLayout;
        grid->addWidget(labelA);
        grid->addWidget(groupBox);
        grid->addWidget(labelB);
        grid->addWidget(buttonBox);

        // Set signal and slot for "Buttons"
        connect(noShutdownButton, SIGNAL(clicked()), this, SLOT(noShutdownButtonClicked()));
        connect(okShutdownButton, SIGNAL(clicked()), this, SLOT(okShutdownButtonClicked()));

        QDialog dlg;
        dlg.setWindowModality(Qt::WindowModal);
        dlg.setLayout(grid);
        dlg.resize(250, 100);
        dlg.setWindowTitle(tr(APP_NAME));
        dlg.exec();
    }
}

void MainWindow::noShutdownButtonClicked()
{

}

void MainWindow::okShutdownButtonClicked()
{

}
