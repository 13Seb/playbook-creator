/** @file mainDialog.cpp
    This file is part of Playbook Creator.

    Playbook Creator is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Playbook Creator is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Playbook Creator.  If not, see <http://www.gnu.org/licenses/>.

    Copyright 2015 Oliver Braunsdorf

    @author Oliver Braunsdorf
*/

#include "mainDialog.h"
#include "ui_mainDialog.h"
#include "QResizeEvent"

#include "util/pbcConfig.h"
#include "gui/pbcPlayerView.h"
#include "models/pbcPlaybook.h"
#include "dialogs/pbcExportPdfDialog.h"
#include "dialogs/pbcNewPlaybookDialog.h"
#include "dialogs/pbcNewPlayDialog.h"
#include "dialogs/pbcOpenPlayDialog.h"
#include "dialogs/pbcSavePlayAsDialog.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QDebug>
#include "util/pbcStorage.h"
#include "util/pbcExceptions.h"
#include <QFileDialog>
#include <QStringList>
#include <QPushButton>
#include <string>
#include <vector>
#include <list>
#include <set>
#include <pbcVersion.h>

/**
 * @class MainDialog
 * @brief This class creates the application's main dialog
 */

/**
 * @brief MainDialog::MainDialog The constructor
 * @param parent The parent widget this dialog belongs to. It should be NULL
 * since this is the main window which contains all other dialogs.
 */
MainDialog::MainDialog(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainDialog),
    _currentPlaybookFileName("") {
    ui->setupUi(this);
    ui->graphicsView->setRenderHint(QPainter::Antialiasing);
    ui->graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    updateTitle(false);
}

/**
 * @brief shows the main window graphically at application startup
 */
void MainDialog::show() {
    QMainWindow::showMaximized();

    unsigned int width = ui->graphicsView->width();
    unsigned int height = ui->graphicsView->height();
    PBCConfig::getInstance()->setCanvasSize(width - 2, height - 2);
    this->setMinimumWidth(PBCConfig::getInstance()->minWidth());
    this->setMinimumHeight(PBCConfig::getInstance()->minHeight());

    _playView = new PBCPlayView(NULL, this);
    ui->graphicsView->setScene(_playView);
    _playView->setSceneRect(0, 0, PBCConfig::getInstance()->canvasWidth(),
                            PBCConfig::getInstance()->canvasHeight());

    QMessageBox messageBox(this);
    QPushButton* openButton = messageBox.addButton("Open Playbook",
                                                  QMessageBox::AcceptRole);
    QPushButton* newButton = messageBox.addButton("New Playbook",
                                                   QMessageBox::AcceptRole);
    messageBox.exec();
    if(messageBox.clickedButton() == newButton) {
        newPlaybook();
    } else if(messageBox.clickedButton() == openButton) {  // NOLINT
        openPlaybook();
    } else {
        assert(false);
    }
}

/**
 * @brief adds / deletes an "*" to / from the window title depending on
 * saved / modified state of the current playbook
 * @param saved true if the playbook is saved, false if it is modified
 */
void MainDialog::updateTitle(bool saved) {
    // TODO(obr): refactor this whole method and logic behind
    std::string windowTitle = "Playbook Creator V" +
            PBCVersion::getSimpleVersionString() +
            " - " +
            PBCPlaybook::getInstance()->name();
    if(saved == false) {
        windowTitle += "*";
    }

    this->setWindowTitle(QString::fromStdString(windowTitle));
}

/**
 * @brief Enables all menu actions.
 *
 * Some of the menu actions are disabled at application startup, because they
 * don't make sense when no playbook is loaded (e.g. action for saving a playbook)
 */
void MainDialog::enableMenuOptions() {
    QList<QAction*> actionList = this->findChildren<QAction*>();
    for(QAction* action : actionList) {
        if(action->isEnabled() == false) {
            action->setEnabled(true);
        }
    }
}


/**
 * @brief A Qt dialog slot which is triggered when the main window is resized.
 *
 * It forces the embedded PBCPlayView to repaint adapting the new size
 * @param e contains resizing meta data
 */
void MainDialog::resizeEvent(QResizeEvent* e) {
    unsigned int width = ui->graphicsView->width();
    unsigned int height = ui->graphicsView->height();
    PBCConfig::getInstance()->setCanvasSize(width - 2, height - 2);
    if(e->oldSize().width() > 0) {
        _playView->setSceneRect(0, 0, PBCConfig::getInstance()->canvasWidth(),
                                PBCConfig::getInstance()->canvasHeight());
        _playView->repaint();
    }
}


/**
 * @brief Terminates the application
 */
void MainDialog::exit() {
    QApplication::quit();
}


/**
 * @brief Loads a new play into the embedded PBCPlayView
 */
void MainDialog::showNewPlay() {
    PBCNewPlayDialog dialog;
    int returnCode = dialog.exec();
    if (returnCode == QDialog::Accepted) {
        PBCNewPlayDialog::ReturnStruct rs = dialog.getReturnStruct();
        std::string name = rs.name;
        std::string codeName = rs.codeName;
        std::string formation = rs.formationName;

        _playView->createNewPlay(name, codeName, formation);

        enableMenuOptions();
    }
}


/**
 * @brief Gets a play by name from the Playbook and loads
 * it into the embedded PBCPlayView
 */
void MainDialog::openPlay() {
    QMessageBox::StandardButton button =
            QMessageBox::warning(this,
                                 "Open Play",
                                 "This will discard unsaved changes to the current play. Do you want to continue?",  // NOLINT
                                 QMessageBox::Ok | QMessageBox::Cancel);
    if(button == QMessageBox::Ok) {
        PBCOpenPlayDialog dialog;
        int returnCode = dialog.exec();
        if (returnCode == QDialog::Accepted) {
            struct PBCOpenPlayDialog::ReturnStruct rs = dialog.getReturnStruct();  // NOLINT
            std::string playName = rs.playName;
            _playView->showPlay(playName);
            updateTitle(true);
            enableMenuOptions();
        }
    }
}


/**
 * @brief Shows a simple dialog with information
 * about the application
 */
void MainDialog::showAboutDialog() {
    QMessageBox::about(this, "About", QString::fromStdString(PBCVersion::getVersionString()).prepend("Playbook Creator by Oliver Braunsdorf\nVersion: ")); //NOLINT
}

/**
 * @brief Saves the current play displayed by the embedded PBCPlayView to the playbook
 */
void MainDialog::savePlay() {
    try {
        _playView->savePlay();
    } catch(const PBCStorageException& e) {
        QMessageBox::information(this, "", "You have to save the playbook to a file before you can add plays");  //NOLINT
        savePlaybookAs();
        return;
    }
    updateTitle(true);
}

/**
 * @brief Saves the current play displayed by the embedded PBCPlayView with a
 * new name to the playbook.
 */
void MainDialog::savePlayAs() {
    PBCSavePlayAsDialog dialog;
    int returnCode = dialog.exec();
    if (returnCode == QDialog::Accepted) {
        struct PBCSavePlayAsDialog::ReturnStruct rs = dialog.getReturnStruct();
        try {
            _playView->savePlay(rs.name, rs.codeName);
        } catch(const PBCStorageException& e) {
            std::string errMsg = "Error saving a play under a different name. Expecting playbook to be saved beforehand.\n";  // NOLINT
            errMsg += e.what();
            throw PBCUnexpectedError(errMsg);
        }
        updateTitle(true);
        _playView->showPlay(rs.name);
    }
}


/**
 * @brief Saves the current formation from the play displayed by the embedded
 * PBCPlayView with a new name to the playbook.
 */
void MainDialog::saveFormationAs() {
    bool ok;
    QString formationName = QInputDialog::getText(
                this, "Save formation as", "formation name",
                QLineEdit::Normal, "", &ok);

    if(ok == true) {
        assert(formationName != "");
        try {
            _playView->saveFormation(formationName.toStdString());
        } catch(const PBCStorageException& e) {
            QMessageBox::information(this, "", "You have to save the playbook to a file before you can add formations");  //NOLINT
            savePlaybookAs();
            return;
        }
    }
}

/**
 * @brief Resets the application and creates a new empty playbook
 */
void MainDialog::newPlaybook() {
    PBCNewPlaybookDialog dialog;
    int returnCode = dialog.exec();
    if (returnCode == QDialog::Accepted) {
        struct PBCNewPlaybookDialog::ReturnStruct rs = dialog.getReturnStruct();
        unsigned int playerNumber = rs.playerNumber;
        std::string name = rs.playbookTitle;

        pbcAssert(name != "");
        pbcAssert(playerNumber == 5 || playerNumber == 7 ||
            playerNumber == 9 || playerNumber == 11);
        PBCPlaybook::getInstance()->resetToNewEmptyPlaybook(name,
                                                            playerNumber);
        PBCStorage::getInstance()->init(name);
        updateTitle(false);
    } else {
        pbcAssert(false);
    }
}

/**
 * @brief Saves the playbook persistently to a file after asking for a file name.
 */
void MainDialog::savePlaybookAs() {
    std::string stdFile = PBCPlaybook::getInstance()->name() + ".pbc";
    QFileDialog fileDialog(
                this, "Save Playbook",
                QString::fromStdString(stdFile),
                "PBC Files (*.pbc);;All Files (*.*)");

    fileDialog.setFileMode(QFileDialog::AnyFile);
    fileDialog.setAcceptMode(QFileDialog::AcceptSave);
    if(fileDialog.exec() == true) {
        QStringList files = fileDialog.selectedFiles();
        assert(files.size() == 1);
        QString fileName = files.first();

        bool ok;
        QString password = QInputDialog::getText(this, "Save Playbook",
                                                 "Enter encryption password",
                                                 QLineEdit::Password, "", &ok);
        if(ok == true) {
            _currentPlaybookFileName = fileName;
            PBCStorage::getInstance()->savePlaybook(password.toStdString(),
                                                    fileName.toStdString());
            updateTitle(true);
        }
    }
}

/**
 * @brief Loads a playbook from a file which is specified by an open-dialog.
 */
void MainDialog::openPlaybook() {
    QFileDialog fileDialog(this, "Open Playbook", "",
                           "PBC Files (*.pbc);;All Files (*.*)");

    fileDialog.setFileMode(QFileDialog::ExistingFile);
    if(fileDialog.exec() == true) {
        QStringList files = fileDialog.selectedFiles();
        assert(files.size() == 1);
        QString fileName = files.first();

        bool ok;
        QString password = QInputDialog::getText(this, "Open Playbook",
                                                 "Enter decryption password",
                                                 QLineEdit::Password, "", &ok);
        if(ok == true) {
            _currentPlaybookFileName = fileName;
            PBCStorage::getInstance()->loadPlaybook(password.toStdString(),
                                                    fileName.toStdString());
            _playView->resetPlay();
            updateTitle(true);
        }
    }
}

/**
 * @brief Exports the playbook to a PDF file.
 *
 * After file name and location have been asked, the PBCExportPDFDialog is
 * invoked to ask for the printing parameters. The PBCStorage is called, which
 * actually implements the PDF printing logic
 *
 */
void MainDialog::exportAsPDF() {
    PBCExportPDFDialog exportDialog;
    exportDialog.setWindowModality(Qt::ApplicationModal);
    boost::shared_ptr<PBCExportPDFDialog::ReturnStruct> returnStruct(new PBCExportPDFDialog::ReturnStruct());  //NOLINT
    boost::shared_ptr<QStringList> playListSP = exportDialog.exec(returnStruct);
    if(playListSP != NULL && playListSP->size() > 0) {
        std::string stdFile = PBCPlaybook::getInstance()->name() + ".pdf";
        QFileDialog fileDialog(
                    this, "Export Playbook As PDF",
                    QString::fromStdString(stdFile),
                    "PDF Documents (*.pdf);;All Files (*.*)");

        fileDialog.setFileMode(QFileDialog::AnyFile);
        fileDialog.setAcceptMode(QFileDialog::AcceptSave);
        if(fileDialog.exec() == true) {
            QStringList files = fileDialog.selectedFiles();
            assert(files.size() == 1);
            QString fileName = files.first();

            PBCStorage::getInstance()->exportAsPDF(fileName.toStdString(),
                                                   playListSP,
                                                   returnStruct->paperWidth,
                                                   returnStruct->paperHeight,
                                                   returnStruct->columns,
                                                   returnStruct->rows,
                                                   returnStruct->marginLeft,
                                                   returnStruct->marginRight,
                                                   returnStruct->marginTop,
                                                   returnStruct->marginBottom);
        }
    }
}


/**
 * @brief Adds the current play to a category.
 *
 */
void MainDialog::addPlayToCategory() {
    try {
        _playView->editCategories();
    } catch(const PBCStorageException& e) {
        QMessageBox::information(this, "", "You have to save the playbook before.");  //NOLINT
        savePlaybookAs();
        return;
    }
}

/**
 * @brief The destructor
 */
MainDialog::~MainDialog() {
    delete ui;
}
