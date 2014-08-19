/*
 * This file is part of the kreenshot-editor project.
 *
 * Copyright (C) 2014 by Gregor Mi <codestruct@posteo.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 or the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */
#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>
#include "ui/mainwindow.h"
#include <kreen/kreenshoteditor.h>

#include <iostream>
#include <stdexcept>

#define tr(arg) QObject::tr(arg)

class Arguments
{
public:
    QString baseImagePath;
    QString description;
    bool treatFileAsNew;
    bool newFromClipboard;
};

Arguments parseArgumentsOrExit(QApplication& app);

int main(int argc, char *argv[])
{
    const QString version = "0.1";
    QApplication app(argc, argv);
    QCoreApplication::setApplicationVersion(version);
    auto arguments = parseArgumentsOrExit(app);

    auto kreenshotEditor = kreen::KreenshotEditor::make_shared();

    kreenshotEditor->setCaptureTime();
    kreenshotEditor->setDescription(arguments.description);
    // TODO later: set pattern from command line (--output-file-pattern)

    if (arguments.newFromClipboard) {
        kreenshotEditor->createNewDocumentFromClipbard();
    }
    else {
        if (arguments.treatFileAsNew) {
            kreenshotEditor->createNewDocument(QImage(arguments.baseImagePath));
        }
        else {
            kreenshotEditor->createNewDocumentFromFile(arguments.baseImagePath);
        }
    }

    MainWindow mainWindow(kreenshotEditor);
    //mainWindow.resize(640, 480);
    mainWindow.show();

    return app.exec();
}

Arguments parseArgumentsOrExit(QApplication& app)
{
    qDebug() << "parseArgumentsOrExit";

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.setApplicationDescription("screenshot image editing, (c) 2014");

    parser.addPositionalArgument("image", tr("file path to primary image to be loaded"), tr("imagefile"));

    QCommandLineOption treatAsNewOption(QStringList() << "treat-as-new", tr("the given primary image path will be used as image data source only and not as save target"));
    parser.addOption(treatAsNewOption);

    QCommandLineOption newFromClipboardOption(QStringList() << "new-from-clipboard", tr("if the clipboard contains image data it will be used as primary image. The --treat-as-new option will be ignored."));
    parser.addOption(newFromClipboardOption);

    QCommandLineOption image2Option(QStringList() << "image2", tr("file path to (optional) secondary image to be inserted as object, e. g. captured mouse cursor"), tr("file path"));
    parser.addOption(image2Option);

    QCommandLineOption image2xOption(QStringList() << "image2x", tr("x coordinate (upper left corner) of the secondary image. Default value is 0."), tr("x in pixel"));
    parser.addOption(image2xOption);

    QCommandLineOption image2yOption(QStringList() << "image2y", tr("y coordinate. Default value is 0."), tr("y in pixel"));
    parser.addOption(image2yOption);

    QCommandLineOption descriptionOption(QStringList() << "description", tr("description, e. g. window title of the captured window"), tr("text"));
    parser.addOption(descriptionOption);

    parser.process(app);

    const QStringList args = parser.positionalArguments();

    Arguments arguments;

    try {
        if (args.length() == 0) {
            arguments.baseImagePath = "";
            qDebug() << "  baseImagePath: no image given";
        }
        else if (args.length() == 1) {
            arguments.baseImagePath = args[0];
            qDebug() << "  baseImagePath:" << arguments.baseImagePath;
        }
        else {
            // http://stackoverflow.com/questions/4214369/how-to-convert-qstring-to-stdstring
            throw std::runtime_error("Zero or one positional argument expected");
        }

        QString image2Filepath = parser.value(image2Option);
        qDebug() << "  image2Filepath:" << image2Filepath;

        QString image2xStr = parser.value(image2xOption);
        qDebug() << "  image2x:" << image2xStr;
        int image2x = 0;
        if (!image2xStr.isEmpty()) {
            bool ok;
            image2x = image2xStr.toInt(&ok);
            if (!ok) throw std::runtime_error(tr("wrong argument (cannot parse x value)").toUtf8().constData());
        }
        qDebug() << "  image2x:" << image2x;

        QString image2yStr = parser.value(image2yOption);
        qDebug() << "  image2y:" << image2yStr;
        int image2y = 0;
        if (!image2yStr.isEmpty()) {
            bool ok;
            image2y = image2yStr.toInt(&ok);
            if (!ok) throw std::runtime_error(tr("wrong argument (cannot parse y value)").toUtf8().constData());
        }
        qDebug() << "  image2y:" << image2y;

        QString descriptionStr = parser.value(descriptionOption);
        arguments.description = descriptionStr;
        qDebug() << "  description:" << arguments.description;

        arguments.treatFileAsNew = parser.isSet(treatAsNewOption);
        qDebug() << "  treatAsNew:" << arguments.treatFileAsNew;

        if (arguments.baseImagePath.isEmpty()) {
            qDebug() << "  treatAsNew: set to true because baseImagePath is empty";
            arguments.treatFileAsNew = true;
        }

        arguments.newFromClipboard = parser.isSet(newFromClipboardOption);
        qDebug() << "  newFromClipboard:" << arguments.newFromClipboard;
    }
    catch (const std::runtime_error& e) {
        std::cout << tr("Error with command line usage: ").toUtf8().constData() << e.what() << std::endl << std::endl;
        parser.showHelp(1); // exits application
    }

    return arguments;
}
