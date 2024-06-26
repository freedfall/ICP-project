/**
 * @file mainwindow.cpp
 * @brief Main file for handling output logic
 * @author Kininbayev Timur (xkinin00)
 * @author Yaroslav Slabik (xslabi01)
 */

#include "mainwindow.h"
#include "obstacle.h"
#include "createobstacledialog.h"
#include "createRobotDialog.h"
#include "ui_mainwindow.h"
#include <QGraphicsScene>
#include <QDebug>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <QString>
#include <QFileDialog>
#include <stdio.h>
#include <QGraphicsDropShadowEffect>
#include <QMessageBox>

/**
 * @brief constructor of the MainWindow class
 *
 * @param parent parent widget
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // slots connections
    connect(ui->stopButton, &QPushButton::clicked, this, &MainWindow::stopSimulation);
    connect(ui->startButton, &QPushButton::clicked, this, &MainWindow::startSimulation);

    connect(ui->importButton, &QPushButton::clicked, this, &MainWindow::onLoadFileClicked);

    connect(ui->createRobot, &QPushButton::clicked, this, &MainWindow::createRobot);
    connect(ui->deleteRobot, &QPushButton::clicked, this, &MainWindow::deleteRobot);

    connect(ui->createObstacle, &QPushButton::clicked, this, &MainWindow::createObstacle);
    connect(ui->deleteObstacle, &QPushButton::clicked, this, &MainWindow::deleteObstacle);

    connect(ui->moveRobotButton, &QPushButton::clicked, this, &MainWindow::moveRobot);
    connect(ui->stopRobotButton, &QPushButton::clicked, this, &MainWindow::stopRobot);

    connect(ui->rotateLeftButton, &QPushButton::clicked, this, &MainWindow::rotateRobotLeft);
    connect(ui->rotateRightButton, &QPushButton::clicked, this, &MainWindow::rotateRobotRight);

    connect(ui->clearButton, &QPushButton::clicked, this, &MainWindow::clearScene);

    ui->startButton->setStyleSheet("QPushButton { background-color: green; }");
    ui->stopButton->setStyleSheet("QPushButton { background-color: red; }");

    this->setMaximumSize(1500, 850);

    // create scene
    QGraphicsScene *scene = new QGraphicsScene(this);
    scene->setSceneRect(0, 0, 1500, 600);
    scene->setBackgroundBrush(QBrush(QColor(51,51,51,200)));

    // create widget and link with scene
    ui->graphicsView->setScene(scene);

    // turn of scrollbars
    ui->graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // ensure that the view stays centered on resize
    ui->graphicsView->setResizeAnchor(QGraphicsView::AnchorViewCenter);
    ui->graphicsView->setTransform(QTransform());

    // set deleting mode to false
    deletingMode = false;
    rDeletingMode = false;

    timer = new QTimer(this);
    timer->setInterval(10); // update every 10 ms
    connect(timer, &QTimer::timeout, this, &MainWindow::updateRobots);

    timer->stop();
}

/**
 * @brief destructor of the MainWindow class
 * 
 */
MainWindow::~MainWindow()
{}

/**
 * @brief Starts or continues the simulation
 *
 */
void MainWindow::startSimulation() {
    if (!timer->isActive()) {
        timer->start();
    }
}

/**
 * @brief Stops the simulation
 *
 */
void MainWindow::stopSimulation() {
    if (timer->isActive()) {
        timer->stop();
    }
}

/**
 * @brief Import test file
 *
 */
void MainWindow::onLoadFileClicked() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("Text Files (*.txt);;All Files (*)"));
    if (!fileName.isEmpty()) {
        loadSceneFromFile(fileName);
    }
}


/**
 * @brief Create obstacle
 *
 */
void MainWindow::createObstacle()
{
    CreateObstacleDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        int x = dialog.getX();
        int y = dialog.getY();
        int width = dialog.getWidth();

        // Creating a QRectF based on obstacle attributes
        QRectF creationObstacleArea(x, y, width, width);

        // Check for overlap with other objects
        QList<QGraphicsItem *> foundItems = ui->graphicsView->scene()->items(creationObstacleArea);
        if (!foundItems.isEmpty()) {
            bool obstacleFound = false;
            bool robotFound = false;
            for (QGraphicsItem *item : foundItems) {
                if (dynamic_cast<Robot*>(item)) {
                    robotFound = true;
                } else if (dynamic_cast<Obstacle*>(item)) {
                    obstacleFound = true;
                }
            }

            QString message = "Cannot place an obstacle here. The space is already occupied by ";
            if (robotFound && obstacleFound) {
                message += "another robot and an obstacle.";
            } else if (robotFound) {
                message += "another robot.";
            } else if (obstacleFound) {
                message += "another obstacle.";
            }

            QMessageBox::warning(this, tr("Placement Error"), tr(message.toStdString().c_str()));
            return;
        }

        Obstacle *obstacle = new Obstacle(x, y, width);
        ui->graphicsView->scene()->addItem(obstacle);
    }
}

/**
 * @brief Delete obstacle
 *
 */
void MainWindow::deleteObstacle()
{
    // set deletingmode flag
    deletingMode = !deletingMode;

    // change visual for deleting mode
    if (deletingMode) {
        foreach (QGraphicsItem *item, ui->graphicsView->scene()->items()) {
            Obstacle *obstacle = dynamic_cast<Obstacle*>(item);
            if (obstacle) {
                obstacle->setPen(QPen(Qt::red));  // set obstacles borders red
            }
        }
    } else {
        foreach (QGraphicsItem *item, ui->graphicsView->scene()->items()) {
            Obstacle *obstacle = dynamic_cast<Obstacle*>(item);
            if (obstacle) {
                obstacle->setPen(QPen(Qt::black));  // return default view
            }
        }
    }
}

/**
 * @brief Create robot
 * @details create robot dialog where user can set robot type, position, orientation, speed and detection radius
 */
void MainWindow::createRobot() {
    CreateRobotDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        int robotType = dialog.getRobotType();
        int orientation = dialog.getOrientation();
        int speed = dialog.getSpeed();
        double detectionRadius = dialog.getDetectionRadius();
        double x = dialog.getX();
        double y = dialog.getY();

        // Determining the size of the robot
        QRectF creationArea(x - 20, y - 20, 40, 40);

        // Overlap check
        QList<QGraphicsItem *> foundItems = ui->graphicsView->scene()->items(creationArea);
        if (!foundItems.isEmpty()) {
            bool obstacleFound = false;
            bool robotFound = false;
            for (QGraphicsItem *item : foundItems) {
                if (dynamic_cast<Robot*>(item)) {
                    robotFound = true;
                } else if (dynamic_cast<Obstacle*>(item)) {
                    obstacleFound = true;
                }
            }

            QString message = "Cannot place a robot here. The space is already occupied by ";
            if (robotFound && obstacleFound) {
                message += "another robot and an obstacle.";
            } else if (robotFound) {
                message += "another robot.";
            } else if (obstacleFound) {
                message += "an obstacle.";
            }

            QMessageBox::warning(this, tr("Error"), tr(message.toStdString().c_str()));
            return;
        }

        if (robotType == 0) {  // Autonomous
            double avoidanceAngle = dialog.getAvoidanceAngle();
            AutonomousRobot *robotItem = new AutonomousRobot(x, y, orientation, detectionRadius, avoidanceAngle, speed);
            autonomousRobots.append(robotItem);
            ui->graphicsView->scene()->addItem(robotItem);
            ui->graphicsView->scene()->update();
        } else {  // Remote Controlled
            RemoteRobot *remoteRobotItem = new RemoteRobot(x, y, speed, detectionRadius);
            remoteRobots.append(remoteRobotItem);
            ui->graphicsView->scene()->addItem(remoteRobotItem);
            ui->graphicsView->scene()->update();
        }
    }
}

/**
 * @brief Delete robot
 */
void MainWindow::deleteRobot()
{
    // set deletingmode flag
    rDeletingMode = !rDeletingMode;

    // change button visual for deletion mode
    if (rDeletingMode) {
        ui->deleteRobot->setStyleSheet("QPushButton { background-color: red; }");
    } else {
        ui->deleteRobot->setStyleSheet("QPushButton { backgorund-color: white; }");
    }
}

/**
 * @brief select remote controlled robot
 *
 * @param robot pointer to remote controlled robot
 */
void MainWindow::selectRobot(RemoteRobot* robot) {
    selectedRobot = robot;  // save selected robot
}

/**
 * @brief move remote controlled robot to it's actual destination
 *
 */
void MainWindow::moveRobot() {
    if (selectedRobot && selectedRobot->scene() && timer->isActive()) {  // Check if selectedRobot is still in the scene
        selectedRobot->moveForward();
    }
}

/**
 * @brief rotate remote controlled robot to the right
 *
 */
void MainWindow::rotateRobotRight() {
    if (selectedRobot && timer->isActive()) {
        selectedRobot->rotateRight();
    }
}

/**
 * @brief rotate remote controlled robot to the left
 *
 */
void MainWindow::rotateRobotLeft() {
    if (selectedRobot && timer->isActive()) {
        selectedRobot->rotateLeft();
    }
}

/**
 * @brief stop remote controlled robot
 *
 */
void MainWindow::stopRobot() {
    if (selectedRobot) {
        selectedRobot->stop();
    }
}

/**
 * @brief Update robots positions
 * Updates all robots in the scene
 */
void MainWindow::updateRobots() {
    ui->graphicsView->scene()->update();
    for (Robot* robot : autonomousRobots) {  // go through all autonomous robots
        robot->update();
    }
    for (Robot* robot: remoteRobots) { // go through all remote controlled robots
        RemoteRobot* remoteRobot = dynamic_cast<RemoteRobot*>(robot);
        if (remoteRobot->getRotationDirection() == RemoteRobot::RotateRight) {
            rotateRobotRight();
        } else if (remoteRobot->getRotationDirection() == RemoteRobot::RotateLeft) {
            rotateRobotLeft();
        }
        if(remoteRobot->isMoving){
            remoteRobot->update();
        }
    }
}

/**
 * @brief Clear scene
 * @details Clear scene from all objects
 * 
 */
void MainWindow::clearScene() {
    ui->graphicsView->scene()->clear(); // delete all objects from scene

    autonomousRobots.clear();
    remoteRobots.clear();

    this->selectedRobot = nullptr;

    ui->graphicsView->scene()->update();
    qDebug() << "Scene cleared";
}

/**
 * @brief Load scene from file
 * @details Load scene from file and create objects based on the file content
 * 
 * @param filename
 */
void MainWindow::loadSceneFromFile(const QString& filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Cannot open file for reading:" << filename;
        return;
    }

    QTextStream in(&file);
    QString buffer;
    QString currentType;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith("#")) {
            continue; // skip blanks and comments
        }

        if (line.endsWith("{")) {
            // new object
            currentType = line.left(line.length() - 1).trimmed();
            buffer.clear();
        } else if (line.startsWith("}")) {
            // end of object
            if (!currentType.isEmpty()) {
                processObject(currentType, buffer);
                currentType.clear();
            }
        } else {
            buffer += line + "\n";
        }
    }
}

/**
 * @brief Process object
 * @details Process object passed from file based on the type and attributes
 * @param type
 * @param attributes 
 */
void MainWindow::processObject(const QString& type, const QString& attributes) {
    QMap<QString, QString> params = parseAttributes(attributes);
    qDebug() << "params: " << params;
    int x = params.value("positionX").toInt();
    qDebug() << "X: " << x;
    int y = params.value("positionY").toInt();
    int speed = params.value("speed").toInt();
    int orientation = params.value("orientation").toInt();
    double detectionRadius = params.value("detectionRadius").toDouble();
    double avoidanceAngle = params.value("avoidanceAngle").toDouble();
    int size = params.value("width").toInt();

    if (type == "AutonomousRobot") {
        AutonomousRobot *robotItem = new AutonomousRobot(x, y, orientation, detectionRadius, avoidanceAngle, speed);
        autonomousRobots.append(robotItem);
        ui->graphicsView->scene()->addItem(robotItem);
        ui->graphicsView->scene()->update();
    } else if (type == "RemoteRobot") {
        RemoteRobot *remoteRobotItem = new RemoteRobot(x, y, speed, detectionRadius);
        remoteRobots.append(remoteRobotItem);
        ui->graphicsView->scene()->addItem(remoteRobotItem);
        ui->graphicsView->scene()->update();
    } else if (type == "Obstacle"){
        Obstacle *obstacle = new Obstacle(x, y, size);
        ui->graphicsView->scene()->addItem(obstacle);
    }
    else {
        qDebug() << "Unknown object type:" << type;
    }
}

/**
 * @brief Parse attributes
 * @details Parse attributes from the file
 * @param attributes 
 * @return QMap<QString, QString> 
 */
QMap<QString, QString> MainWindow::parseAttributes(const QString& attributes) {
    QMap<QString, QString> params;
    QStringList rawLines = attributes.split("\n");
    QStringList lines;

    // Manually remove empty lines
    for (const QString& line : rawLines) {
        if (!line.trimmed().isEmpty()) {
            lines.append(line);
        }
    }

    // Parse key-value pairs
    for (const QString& line : lines) {
        QStringList parts = line.split("=");
        if (parts.size() == 2) {
            QString key = parts[0].trimmed();
            QString value = parts[1].trimmed();
            params[key] = value;
        }
    }
    return params;
}




