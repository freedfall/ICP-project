/**
 * @file robots.h
 * @author Yaroslav Slabik (xslabi01)
 * @author Kininbayev Timur (xkinin00)
 * @brief File containing the robot class logic
 */
#include "qdebug.h"
#include "qgraphicsscene.h"
#include <iostream>
#include <cmath>  // for basic math functions such as cos() and sin()
#include <QGraphicsItem>
#include <QPainter>

/**
 * @class Robot
 * @brief Abstract class for the robot object
 */
class Robot : public QGraphicsItem {
protected:
    double positionX, positionY;
    int speed;
    QColor color;
public:
    bool isMoving = false;
    /**
     * @brief enum for the rotation direction
     * 
     */
    enum RotationDirection {
        NoRotation,
        RotateLeft,
        RotateRight
    };

    Robot(double posX, double posY, int speed)
        : positionX(posX), positionY(posY), speed(speed){}

    QRectF boundingRect() const override {
        return QRectF(-20, -20, 40, 40);  // robot size
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override {
        Q_UNUSED(option);
        Q_UNUSED(widget);

        painter->setBrush(Qt::blue);  // set blue color for the robot
        painter->drawEllipse(positionX, positionY, 20, 20);
    }

    QPointF rotatePoint(const QPointF& point, double angle) {
        return QPointF(cos(angle) * point.x() - sin(angle) * point.y(),
                       sin(angle) * point.x() + cos(angle) * point.y());
    }

    void setColor(const QColor &newColor) {
        if (color != newColor) {  // change color only if it's different from actual
            color = newColor;
            QGraphicsItem::update();  // call base update method for color changing
        }
    }

    virtual void update() = 0; // Pure virtual function for updating the robot's position
};

/**
 * @class AutonomousRobot
 * @brief Class for the autonomous robot object
 */
class AutonomousRobot : public Robot {
private:
    double detectionRadius;  // Radius in which the robot detects obstacles
    double avoidanceAngle;  // Angle to turn for obstacle avoidance
    int angle;
    int orientation;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
public:
    AutonomousRobot(double posX, double posY, int orient, double detectRadius, double avoidAngle, int speed)
        : Robot(posX, posY, speed), detectionRadius(detectRadius), avoidanceAngle(avoidAngle) {
        color = Qt::blue;
        angle = avoidAngle;
        switch (orient) {
            case 0: orientation = 270; break; // top
            case 1: orientation = 0;   break; // right
            case 2: orientation = 90;  break; // bottom
            case 3: orientation = 180; break; // left
            default: orientation = 0;  break; // default right
        }
        setPos(posX, posY);
    }
    void move();
    bool detectObstacle();

    /**
     * @brief Update the robot's positions
     * @details using interpolation to move robot only 10% of it's speed per tick
     */
    void update() override {
        // Calculate proposed movement
        double radAngle = orientation * M_PI / 180;
        double targetX = positionX + speed * cos(radAngle);
        double targetY = positionY + speed * sin(radAngle);

        // using interpolation to move robot only 10% of it's speed per tick
        positionX += 0.1 * (targetX - positionX);
        positionY += 0.1 * (targetY - positionY);

        // Normalize orientation
        if (orientation < 0) orientation += 360;
        if (orientation >= 360) orientation -= 360;

        setPos(positionX, positionY);
        if (detectObstacle()) {
            orientation += avoidanceAngle; // Update orientation to avoid collision
        }

        qDebug() << "Robot moved to:" << positionX << "," << positionY;
}

    /**
     * @brief Paint the robot and its field of vision
     *
     * @param painter
     * @param option
     * @param widget
     */
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override{
        Q_UNUSED(option);
        Q_UNUSED(widget);

        // Basic robot visualization
        painter->setBrush(color);
        painter->drawEllipse(boundingRect()); // Draw robot centered at its position

        // Variables for trapezoid field of vision
        double radOrientation = orientation * M_PI / 180;
        double robotRadius = 20;  // Radius of the robot
        double halfTopWidth = detectionRadius * tan(M_PI / 6); // Half width at the detection radius
        double halfBaseWidth = halfTopWidth / 4;  // Half width at the robot
        if (halfBaseWidth > robotRadius / 3){
            halfBaseWidth = robotRadius / 3;
        }

        // Calculate trapezoid corners
        QPointF baseLeft(robotRadius, -halfBaseWidth);  // Start from the right edge of the robot
        QPointF baseRight(robotRadius, halfBaseWidth);
        QPointF topRight(detectionRadius + robotRadius, halfTopWidth);
        QPointF topLeft(detectionRadius + robotRadius, -halfTopWidth);

        // Rotate points around the robot's center at (0,0)
        baseLeft = rotatePoint(baseLeft, radOrientation);
        baseRight = rotatePoint(baseRight, radOrientation);
        topRight = rotatePoint(topRight, radOrientation);
        topLeft = rotatePoint(topLeft, radOrientation);

        // Creating the path for the trapezoid field of vision
        QPainterPath viewField;
        viewField.moveTo(baseLeft);
        viewField.lineTo(baseRight);
        viewField.lineTo(topRight);
        viewField.lineTo(topLeft);
        viewField.closeSubpath();

        // Setting semi-transparent red for the field of vision
        painter->setBrush(QColor(255, 0, 0, 100));
        painter->drawPath(viewField);
    }

    /**
     * @brief Rotate the robot by a given angle
     *
     * @param angle
     */
    void rotate(double angle) {
        setRotation(angle);  // Rotate the robot
    }
};

/**
 * @class RemoteRobot
 * @brief Class for the remote robot object
 */
class RemoteRobot: public Robot {
private:
    int orientation = 0;
    double detectionRadius;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
public:
    RemoteRobot(double posX, double posY, int speed, double detectionRadius)
        : Robot(posX, posY, speed), detectionRadius(detectionRadius) {
        setPos(positionX, positionY);
        color = Qt::magenta;
    }
    RotationDirection rotationDirection = NoRotation;

    void moveForward();
    void rotateRight();
    void rotateLeft();
    void stop();
    void updatePosition();
    bool detectObstacle();
    RotationDirection getRotationDirection();

    /**
     * @brief Update the robot's position
     *
     */
    void update() override {
        // Calculate proposed movement
        double radAngle = orientation * M_PI / 180;
        double targetX = positionX + speed * cos(radAngle);
        double targetY = positionY + speed * sin(radAngle);

        // interpolation
        positionX += 0.1 * (targetX - positionX);
        positionY += 0.1 * (targetY - positionY);
        setPos(positionX, positionY);

        if (detectObstacle()) {
            stop();
        }

        qDebug() << "Robot moved to:" << positionX << "," << positionY;
    }

    /**
     * @brief Paint the robot and its field of vision
     *
     * @param painter
     * @param option
     * @param widget
     */
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override{
        Q_UNUSED(option);
        Q_UNUSED(widget);

        // Basic robot visualization
        painter->setBrush(color);
        painter->drawEllipse(boundingRect()); // Draw robot centered at its position

        // Variables for trapezoid field of vision
        double radOrientation = orientation * M_PI / 180;
        double robotRadius = 20;  // Radius of the robot
        double halfTopWidth = detectionRadius * tan(M_PI / 6); // Half width at the detection radius
        double halfBaseWidth = halfTopWidth / 4;  // Half width at the robot
        if (halfBaseWidth > robotRadius / 3){
            halfBaseWidth = robotRadius / 3;
        }

        // Calculate trapezoid corners
        QPointF baseLeft(robotRadius, -halfBaseWidth);  // Start from the right edge of the robot
        QPointF baseRight(robotRadius, halfBaseWidth);
        QPointF topRight(detectionRadius + robotRadius, halfTopWidth);
        QPointF topLeft(detectionRadius + robotRadius, -halfTopWidth);

        // Rotate points around the robot's center at (0,0)
        baseLeft = rotatePoint(baseLeft, radOrientation);
        baseRight = rotatePoint(baseRight, radOrientation);
        topRight = rotatePoint(topRight, radOrientation);
        topLeft = rotatePoint(topLeft, radOrientation);

        // Creating the path for the trapezoid field of vision
        QPainterPath viewField;
        viewField.moveTo(baseLeft);
        viewField.lineTo(baseRight);
        viewField.lineTo(topRight);
        viewField.lineTo(topLeft);
        viewField.closeSubpath();

        // Setting semi-transparent red for the field of vision
        painter->setBrush(QColor(255, 0, 0, 100));
        painter->drawPath(viewField);
    }
};
