/**
 * @file obstacle.h
 * @author Kininbayev Timur (xkinin00)
 * @brief File containing the obstacle class declaration
 * @version 0.1
 * @date 2024-05-02
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef OBSTACLE_H
#define OBSTACLE_H

#include <QGraphicsRectItem>

/**
 * @class Obstacle
 * @brief Class for the obstacle object
 * 
 */
class Obstacle : public QGraphicsRectItem
{
public:
    Obstacle(qreal x, qreal y, qreal width, QGraphicsItem *parent = nullptr);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;  // mouse press event handler

#endif // OBSTACLE_H
};
