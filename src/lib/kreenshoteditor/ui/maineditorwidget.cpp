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
#include "maineditorwidget.h"
#include "core/document.h"
#include "core/documentfile.h"
#include <QPainter>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsItem>
#include <QMessageBox>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QGridLayout>
#include <QDebug>
#include <QMouseEvent>
#include <QMargins>
#include <QImageReader>
#include <QImageWriter>
#include <QTimer>
#include <memory>
#include "../core/impl/kreengraphicsitems.h"
#include "../core/impl/kreengraphicsscene.h"
#include "../core/impl/toolmanager.h"
#include "impl/kreengraphicsview.h"
#include "../core/impl/selectionhandles.h" // TODO: move to ui/impl?

namespace kreen {
namespace ui {


//class ItemVisual {
//public:
// bool mouseOver; // TODO later
//};

class ImageOperationHandling
{
public:
    /**
     * not nullptr if there is an image operation item (like crop) waiting for the user
     */
    QGraphicsItem* imageOperationGraphicsItem; // TODO: rename to imgOpGraphicsItem
    KreenItemPtr imageOperationItem; // imgOpItem

    bool imageOperationItemActive()
    {
        return imageOperationGraphicsItem != nullptr;
    }
};

class MainEditorWidgetImpl
{
public:
    MainEditorWidget* _owner;
    KreenshotEditorPtr kreenshotEditor() { return _kreenshotEditor; };
    ToolManagerPtr toolManager_;
    KreenGraphicsViewPtr graphicsView;
    QGraphicsPixmapItem* baseImageSceneItem;
    ImageOperationHandling imgOpHandling;
    SelectionHandlesPtr selectionHandles;

private:
    KreenshotEditorPtr _kreenshotEditor;

public:
    MainEditorWidgetImpl(MainEditorWidget* owner)
    {
        _owner = owner;
        imgOpHandling.imageOperationGraphicsItem = nullptr;
    }

    void init(KreenshotEditorPtr kreenshotEditor_)
    {
        _kreenshotEditor = kreenshotEditor_;
        selectionHandles = std::make_shared<SelectionHandles>(scene().get()); // needs kreenshotEditor
    }

//     std::map<ItemPtr, bool> mouseOverMap; // TODO later
//     const int mouseOverMargin = 2; // TODO later

public:
    // todo: optimize this method?
    QRect getBaseRect() {
        QImage baseImage = _kreenshotEditor->documentFile()->document()->baseImage();
        QRect rect(0, 0, baseImage.width(), baseImage.height());
        qDebug() << rect;
        return rect;
    }

    KreenGraphicsScenePtr scene()
    {
        return _kreenshotEditor->documentFile()->document()->graphicsScene();
    }

    ToolManagerPtr toolManager()
    {
        return toolManager_;
    }

    // todo: remove later
//             auto mouseOverMapItem = mouseOverMap.find(item);
//             bool isMouseOver = mouseOverMapItem != mouseOverMap.end() && mouseOverMapItem->second == true;
//
//             // highlight: // TODO: for "drawRoundRect" move this away from the scene but directly to the painter!
//             if (isMouseOver && !isSelected) {
//                 //qDebug() << "item: " << item->rect() << "mouseOverMapItem";
//
//                 // removes any other effect which is not desired:
//                 //auto effect = new QGraphicsColorizeEffect();
//                 //effect->setColor(Qt::blue);
//                 //grItem->setGraphicsEffect(effect);
//
//                 auto rectItem = new QGraphicsRectItem();
//                 rectItem->setRect(item->rect().marginsAdded(QMargins(mouseOverMargin, mouseOverMargin, mouseOverMargin, mouseOverMargin)));
//                 //rectItem->setPen(QPen(Qt::gray, 2, Qt::DotLine, Qt::RoundCap, Qt::RoundJoin));
//                 rectItem->setPen(QPen(Qt::transparent));
//                 rectItem->setBrush(QBrush(Qt::Dense7Pattern));
//                 scene.addItem(rectItem);
//             }
//
//             // selected indicator: TODO: draw handles (as extra class?)
//             if (isSelected) {
//                 auto rectItem = new QGraphicsRectItem();
//                 rectItem->setRect(item->rect().marginsAdded(QMargins(mouseOverMargin, mouseOverMargin, mouseOverMargin, mouseOverMargin)));
//                 rectItem->setPen(QPen(Qt::darkGray, 2, Qt::DotLine, Qt::RoundCap, Qt::RoundJoin));
//                 //rectItem->setBrush(QBrush(Qt::Dense7Pattern));
//                 scene.addItem(rectItem);
//             }


    /**
     * all corresponding graphics items for all KreenItems (including op- items like crop or rip out!)
     */
    QList<KreenQGraphicsItemBase*> kreenGraphicsItems()
    {
        QList<KreenQGraphicsItemBase*> list;

        foreach(auto grItem, graphicsView->items()) {

            auto grItemBase = dynamic_cast<KreenQGraphicsItemBase*>(grItem);
            if (grItemBase != nullptr) { // there might also be other items
                list << grItemBase;
            }
        }

        return list;
    }

    /**
     * update positions and sizes from model
     */
    void slotUpdateItemsGeometryFromModel()
    {
        foreach(auto grItem, graphicsView->items()) {
            //qDebug() << "muh";

            auto grItemBase = dynamic_cast<KreenQGraphicsItemBase*>(grItem);
            if (grItemBase != nullptr) { // there might also be other items

                //if (!grItemBase->item()->typeId.startsWith("op-")) {
                //qDebug() << "updateGeometryFromModel";
                grItemBase->updateVisualGeometryFromModel();
//                 }
//                 else {
//                     qDebug() << "updateItemsGeometryFromModel: ignore because of op- item";
//                 }
            }
        }
    }

    void updateItemsBehaviourFromChosenTool()
    {
        foreach(auto grItem, graphicsView->items()) {
            //qDebug() << "muh";
            auto grItemBase = dynamic_cast<KreenQGraphicsItemBase*>(grItem);
            if (grItemBase != nullptr) { // there might also be other items
                grItemBase->setMovable(toolManager()->chosenTool() == ToolEnum::Select);
            }
        }
    }

    void updateDragModeFromChosenTool()
    {
        if (toolManager()->chosenTool() == ToolEnum::Select) {
            graphicsView->setDragMode(QGraphicsView::RubberBandDrag);
            //graphicsView->setDragMode(QGraphicsView::ScrollHandDrag); // todo later: use this when press control?
        }
        else {
            graphicsView->setDragMode(QGraphicsView::NoDrag);
        }
    }

    void createDemoScene()
    {
        QRect rect = getBaseRect();
        scene()->setSceneRect(rect);

        {
            auto dropShadow = new QGraphicsDropShadowEffect();
            dropShadow->setColor(Qt::black);
            dropShadow->setOffset(QPoint(3, 3));
            dropShadow->setBlurRadius(10);

            auto rectItem = new QGraphicsRectItem();
            rectItem->setRect(120, 100, 150, 100);
            rectItem->setGraphicsEffect(dropShadow);
            rectItem->setPen(QPen(Qt::darkGreen, 3, Qt::DotLine, Qt::RoundCap, Qt::RoundJoin));
            scene()->addItem(rectItem);
        }

        {
            auto rect2Item = new QGraphicsRectItem();
            rect2Item->setRect(10, 5, 30, 40);
            //rect2Item->setGraphicsEffect(dropShadow);
            //rect2Item->setPen(QPen(Qt::darkGreen, 3, Qt::DotLine, Qt::RoundCap, Qt::RoundJoin));
            scene()->addItem(rect2Item);
        }

        scene()->addText("Hello, world!");

        {
            auto dropShadow = new QGraphicsDropShadowEffect();
            dropShadow->setColor(Qt::black);
            dropShadow->setOffset(QPoint(2, 2));
            dropShadow->setBlurRadius(5);

            auto textItem = new QGraphicsTextItem("With drop shadow");
            textItem->setPos(30, 60);
            textItem->setGraphicsEffect(dropShadow);
            scene()->addItem(textItem);
        }

        {
            auto textItem = new QGraphicsTextItem("TODO: With white glow background");
            textItem->setPos(30, 80);
            scene()->addItem(textItem);
        }
    }
};

MainEditorWidget::MainEditorWidget(KreenshotEditorPtr kreenshotEditor)
{
    KREEN_PIMPL_INIT_THIS(MainEditorWidget);
    d->init(kreenshotEditor);
    d->toolManager_ = std::make_shared<ToolManager>();
    d->kreenshotEditor()->documentFile()->document()->graphicsScene()->setToolManager(d->toolManager());

    bool oldScrollAreaCode = false;

    if (oldScrollAreaCode) {
        // for QScrollArea:
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        setMinimumSize(d->getBaseRect().size());
    }
    else {
        // use this if not using QScrollArea:
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setMinimumSize(50, 50);
    }

    setMouseTracking(true); // to enable mouseMoveEvent

    auto layout = new QGridLayout();
    this->setLayout(layout);
    d->graphicsView = std::make_shared<KreenGraphicsView>(d->toolManager());
    //d->graphicsView->setRubberBandSelectionMode(Qt::IntersectsItemShape); // default
    layout->addWidget(d->graphicsView.get(), 0, 0);
    layout->setMargin(0);

    //d->createDemoScene();
    d->kreenshotEditor()->documentFile()->document()->addDemoItems();
    initScene();

    // makes sure that every time the mouse is released the whole scene is update from model
    // to check if everything is ok (e. g. with multiselection moves)
    connect(d->scene().get(), SIGNAL(mouseReleased()), this, SLOT(slotUpdateItemsGeometryFromModel()));

    connect(d->scene().get(), SIGNAL(itemCreated(KreenItemPtr)), this, SLOT(slotHandleNewItem(KreenItemPtr)));

    connect(d->scene().get(), SIGNAL(selectionChanged()), this, SLOT(slotSceneSelectionChanged()));
}

MainEditorWidget::~MainEditorWidget()
{

}

void MainEditorWidget::initScene() {
    QRect rect = d->getBaseRect();
    d->scene()->setSceneRect(rect);

    d->graphicsView->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    // WORKAROUND:
    d->graphicsView->setSceneRect(0, 0, 10, 10); // this makes sure that the view scrolls to 0, 0
    d->graphicsView->setScene(d->scene().get());
    d->graphicsView->setSceneRect(rect); // this makes sure the scroll bars are shown for large images

    // scroll to 0,0 / does all not work:
    //graphicsView->scroll(-100, -100);
    // graphicsView->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    // graphicsView->ensureVisible(0, 0, 1, 1);

    createSceneFromModel();
}

/**
 * recreate the scene to reflect the current kreenshotEditor->documentFile()->document()
 */
void MainEditorWidget::createSceneFromModel(KreenItemPtr selectNewItem /*= nullptr*/)
{
    d->scene()->clear();

    QPixmap pixmap;
    pixmap.convertFromImage(d->kreenshotEditor()->documentFile()->document()->baseImage());
    d->baseImageSceneItem = new QGraphicsPixmapItem(pixmap);
    d->graphicsView->setHelperBaseImageItem(d->baseImageSceneItem);
    d->scene()->addItem(d->baseImageSceneItem);

    QGraphicsItem* selectNewItemGrItem = nullptr;

    foreach (KreenItemPtr item, d->kreenshotEditor()->documentFile()->document()->items()) {

        auto grItem = d->toolManager()->createGraphicsItemFromKreenItem(item, d->scene().get());
        auto grItemBase = dynamic_cast<KreenQGraphicsItemBase*>(grItem);

        connect(grItemBase, SIGNAL(itemPositionHasChangedSignal()), this, SLOT(slotRedrawSelectionHandles()));

        d->scene()->addItem(grItem);

        if (selectNewItem == item) {
            selectNewItemGrItem = grItem;
        }
    }

    d->slotUpdateItemsGeometryFromModel();
    d->updateItemsBehaviourFromChosenTool();

    // make item selectable AFTER calling updateItemsBehaviourFromChosenTool() because we might override
    if (selectNewItemGrItem != nullptr) {
        selectNewItemGrItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
        selectNewItemGrItem->setSelected(true);
        qDebug() << "isSelected: " << selectNewItemGrItem->isSelected();
    }
}

void MainEditorWidget::slotUpdateSceneWithImageOperationItem(KreenItemPtr item)
{
    qDebug() << "updateSceneWithImageOperationItem";

    d->imgOpHandling.imageOperationItem = item;
    d->toolManager()->setImageOperationActive(item != nullptr);

    if (d->imgOpHandling.imageOperationGraphicsItem != nullptr) {
        d->scene()->removeItem(d->imgOpHandling.imageOperationGraphicsItem);
        d->imgOpHandling.imageOperationGraphicsItem = nullptr;
        d->imgOpHandling.imageOperationItem = nullptr;
    }

    if (item != nullptr) {

//             auto dimRect = new QGraphicsRectItem();
//             //dimRect->setBrush();
//             scene.addItem(dimRect);

        auto grItem = d->toolManager()->createGraphicsItemFromKreenItem(item, d->scene().get());
        d->scene()->addItem(grItem);
        d->imgOpHandling.imageOperationGraphicsItem = grItem;
        auto grItemBase = dynamic_cast<KreenQGraphicsItemBase*>(grItem);
        grItemBase->setIsCreating(false);
        grItemBase->updateVisualGeometryFromModel();
        connect(grItemBase, SIGNAL(operationAccepted()), this, SLOT(slotImageOperationAccepted()));
        connect(grItemBase, SIGNAL(operationCanceled()), this, SLOT(slotImageOperationCanceled()));
    }
}

void MainEditorWidget::paintEvent(QPaintEvent* event)
{
    // QPainter painter(this);
    QWidget::paintEvent(event);
}

// void setCursorFromChosenTool(ToolManagerPtr toolManager, QGraphicsItem* imageItem)
// {
//     // workaround (not really) for https://bugreports.qt-project.org/browse/QTBUG-4190
//     //QWidget* w = imageItem;
//     QCursor curCursor = imageItem->cursor();
//     Qt::CursorShape newCursorShape;
//     auto tool = toolManager->chosenTool();
//
//     if (tool == Select) {
//         newCursorShape = Qt::ArrowCursor;
//     }
//     else if (tool == DrawRect) {
//         newCursorShape = Qt::CrossCursor;
//     }
//     else if (tool == DrawLine) {
//         newCursorShape = Qt::CrossCursor;
//     }
//     else if (tool == DrawEllipse) {
//         newCursorShape = Qt::CrossCursor;
//     }
//     else if (tool == DrawText) {
//         newCursorShape = Qt::CrossCursor;
//     }
//     else if (tool == OperationCrop) {
//         newCursorShape = Qt::CrossCursor;
//     }
//     else {
//         qDebug() << "_chosenTool" << tool;
//         Q_ASSERT(false);
//     }
//
//     if (curCursor.shape() != newCursorShape) {
//         auto tool = toolManager->chosenTool();
//         qDebug() << QTime::currentTime() << " baseImage setCursor for " << tool << "cursor=" << newCursorShape;
//         imageItem->setCursor(newCursorShape);
//     }
// }


void MainEditorWidget::requestTool(QString toolId)
{
    ToolEnum toolEnum;

    if (toolId == "select") {
        toolEnum = ToolEnum::Select;
    }
    else if (toolId == "rect") {
        toolEnum = ToolEnum::DrawRect;
    }
    else if (toolId == "ellipse") {
        toolEnum = ToolEnum::DrawEllipse;
    }
    else if (toolId == "line") {
        toolEnum = ToolEnum::DrawLine;
    }
    else if (toolId == "op-crop") {
        toolEnum = ToolEnum::OperationCrop;
    }
    else {
        toolEnum = ToolEnum::Select;
        toolId = "select";

        QString message = QString("TODO / Unknown tool id '%1'").arg(toolId);
        qDebug() << message;
        QMessageBox::information(this, "Not impl", message);
    }

    d->toolManager()->setChosenTool(toolEnum, this);
    d->updateItemsBehaviourFromChosenTool();

    // remove current image operation if another tool is selected
    if (!toolId.startsWith("op-")) {
        slotUpdateSceneWithImageOperationItem(nullptr);
    }

    d->graphicsView->setCursorFromChosenTool();
    d->updateDragModeFromChosenTool();
    emit toolChosenSignal(toolId);
}

void MainEditorWidget::slotUpdateItemsGeometryFromModel()
{
    qDebug() << "updateItemsGeometryFromModel";
    d->slotUpdateItemsGeometryFromModel();
    d->selectionHandles->redrawSelectionHandles(true);
}

void MainEditorWidget::slotImageOperationAccepted()
{
    qDebug() << "MainEditorWidget::imageOperationAccepted(). Forward to imageOperationAcceptedDecoupled() because otherwise some mouse release event will crash because image operation object will be remove";
    QTimer::singleShot(0, this, SLOT(slotImageOperationAcceptedDecoupled()));
}

void MainEditorWidget::slotImageOperationAcceptedDecoupled()
{
    qDebug() << "MainEditorWidget::imageOperationAcceptedDecoupled()";
    d->kreenshotEditor()->documentFile()->document()->operationCrop(d->imgOpHandling.imageOperationItem->rect());

    slotUpdateSceneWithImageOperationItem(nullptr); // remove image operation item
    initScene(); // would causes crash in mouse event if not called in the decoupled method
    requestTool("select"); // go to Select after an image operation
}


void MainEditorWidget::slotImageOperationCanceled()
{
    slotUpdateSceneWithImageOperationItem(nullptr);
}

void MainEditorWidget::slotHandleNewItem(KreenItemPtr item)
{
    qDebug() << "add item: " << item->rect();
    if (!item->typeId.startsWith("op-")) {
        d->kreenshotEditor()->documentFile()->document()->addItem(item);
        createSceneFromModel(item);
    }
    else {
        slotUpdateSceneWithImageOperationItem(item);
    }
}

void MainEditorWidget::slotSceneSelectionChanged()
{
    qDebug() << "sceneSelectionChanged() " << d->scene()->selectedItems();

    qDebug() << "[DEBUG] d->graphicsView->unsetCursor() (does not work yet). See comment in code.";
    d->graphicsView->unsetCursor(); // DETAIL: to have the cursor correct on the selection handle without having to move the mouse

    d->selectionHandles->redrawSelectionHandles(true);

    emit itemSelectionChanged();
}

void MainEditorWidget::slotRedrawSelectionHandles()
{
    //qDebug() << "SLOT redrawSelectionHandles";
    d->selectionHandles->redrawSelectionHandles(false);
}

void MainEditorWidget::deleteSelectedItems()
{
    QList<KreenItemPtr> toBeErased;
    foreach (auto grItem, d->scene()->selectedItems()) {

        auto kGrItem = dynamic_cast<KreenQGraphicsItemBase*>(grItem);
        toBeErased.append(kGrItem->item());
    }

    d->kreenshotEditor()->documentFile()->document()->removeItems(toBeErased);

    createSceneFromModel();
}

void MainEditorWidget::selectAllItems()
{
    requestTool("select");
    
    // todo: potential optimization: disable events before the loop and enable afterwards
    foreach(auto kreenGraphicsItemBase, d->kreenGraphicsItems()) {
        kreenGraphicsItemBase->graphicsItem()->setSelected(true);
    }
}

int MainEditorWidget::selectedItemsCount()
{
    return d->scene()->selectedItems().size();
}

}
}
