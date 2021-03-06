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
#include "document.h"
#include <QImage>
#include <QDebug>
#include <QPainter>
#include <QApplication>
#include <QClipboard>
#include <QUndoStack>
#include <QThread>
#include "impl/undocommands.h"

namespace kreen {

namespace ui {
    KREEN_SHAREDPTR_FORWARD_DECL(KreenGraphicsScene)
}

namespace core {

class Document::Private
{
public:
    Private(Document* q) : q_ptr(q)
    {
    }

    /**
     * this image serves as a placeholder image if no base image is loaded
     */
    static QImage createDefaultImage()
    {
        QPixmap pixmap(400, 300);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBackground(QBrush(Qt::white));
        painter.setBrush(QBrush(Qt::white));
        painter.setPen(QPen(Qt::white));
        painter.drawRect(0, 0, 400, 300);
        //painter.setBrush(QBrush(Qt::lightGray));
        painter.setPen(QPen(Qt::gray));
        painter.setFont(QFont("Sans", 16));
        painter.drawText(QPointF(30.0, 50.0), "No image data present:");
        painter.drawText(QPointF(30.0, 90.0), "Default image data.");
        return pixmap.toImage();
    }

    bool contentChangedNotificationGroupActive()
    {
        // qDebug() << "contentChangedNotificationGroupActive" << contentChangedNotificationGroupDepth;
        return contentChangedNotificationGroupDepth > 0;
    }

    // needed at all?
//     void contentChangedNotificationGroupMethodEnter(bool recordUndo)
//     {
//         if (contentChangedNotificationGroupActive()) {
//             Q_ASSERT(contentChangedNotificationGroupRecordUndo == recordUndo);
//         }
//     }

    /**
     * emits the contentChangedSignal() if group is closed
     */
    void contentChangedNotificationGroupMethodLeave()
    {
        if (!contentChangedNotificationGroupActive()) {
            emit q_ptr->contentChangedSignal();
        }
    }

    int maxZValue() {
        if (q_ptr->items().empty()) {
            return -1;
        }

        return q_ptr->items().last()->zValue();
    }

public:
    /**
     * base/background image that also defines the dimensions of the editing area
     */
    QImage baseImage;

    /**
     * All items of the document.
     * map item id --> Item
     * With QMap, the items are always sorted by key.
     * The z-order is determined by an attribute of KreenItem
     */
    QMap<int, KreenItemPtr> itemMap;

    /**
     * simple id generator (will be just incremeted)
     */
    int idGenerator = 0;

    /**
     * holds the latest copy of the items of itemMap, sorted by z-order
     */
    QList<KreenItemPtr> itemsCache;
    bool itemsCacheDirty = true;

    QUndoStack undoStack;
    /**
     * greater than 0 if during a contentChangedNotificationGroup
     */
    int contentChangedNotificationGroupDepth = 0;
    // no default value because value is only valid if contentChangedNotificationGroupDepth > 0
    bool contentChangedNotificationGroupRecordUndo;

    /**
     * see renderToImage()
     */
    QImage renderToImageResult;

private:
    Document* q_ptr = nullptr;
};

DocumentPtr Document::make_shared(QImage baseImage)
{
    return std::make_shared<Document>(baseImage);
}

Document::Document(QImage baseImage) : d_ptr(new Private(this))
{
    Q_D();

    if (baseImage.isNull()) {
        baseImage = Document::Private::createDefaultImage();
    }

    setBaseImage(baseImage, false);

    setClean(); // a new document with a base image has to be clean

    // Will emit contentChangedSignal; this might lead to duplicate emitting of contentChangedSignal
    // but this can be dealt later if it even gets important
    connect(&d->undoStack, SIGNAL(cleanChanged(bool)), this, SLOT(slotContentChangedSignalEmitter()));
}

Document::~Document()
{
    delete d_ptr;
}

QImage Document::baseImage()
{
    Q_D();
    return d->baseImage;
}

void Document::setBaseImage(QImage image, bool recordUndo)
{
    Q_D();
    if (recordUndo) {
        d->undoStack.push(new SetBaseImageCmd(this, image)); // this will call setBaseImage with recordUndo=false
    }
    else {
        //qDebug() << "Document::setBaseImage: d->baseImage = image";
        d->baseImage = image;
    }
}

bool Document::isClean()
{
    Q_D();
    return d->undoStack.isClean();
}

void Document::setClean()
{
    Q_D();
    d->undoStack.setClean();
}

void Document::undo()
{
    Q_D();
    contentChangedNotificationGroupBegin(false);
    qDebug() << "Document::undo(), " << d->undoStack.undoText();
    d->undoStack.undo();
    contentChangedNotificationGroupEnd();
}

void Document::redo()
{
    Q_D();
    contentChangedNotificationGroupBegin(false);
    qDebug() << "Document::redo(), " << d->undoStack.redoText();
    d->undoStack.redo();
    contentChangedNotificationGroupEnd();
}

bool Document::canUndo()
{
    Q_D();
    return d->undoStack.canUndo();
}

bool Document::canRedo()
{
    Q_D();
    return d->undoStack.canRedo();
}

void Document::contentChangedNotificationGroupBegin(bool recordUndo, QString undoMacroText)
{
    Q_D();
    if (d->contentChangedNotificationGroupDepth > 0) { // 2nd or xth call
        Q_ASSERT(d->contentChangedNotificationGroupRecordUndo == recordUndo);
    }

    d->contentChangedNotificationGroupDepth++;
    d->contentChangedNotificationGroupRecordUndo = recordUndo;

    if (recordUndo) {
        d->undoStack.beginMacro(undoMacroText);
    }
}

void Document::contentChangedNotificationGroupEnd()
{
    Q_ASSERT(d->contentChangedNotificationGroupDepth > 0);

    Q_D();

    d->contentChangedNotificationGroupDepth--;

    if (d->contentChangedNotificationGroupRecordUndo) {
        d->undoStack.endMacro();
    }

    if (d->contentChangedNotificationGroupDepth == 0) {
        emit contentChangedSignal();
    }
}

// int Document::contentHashTransient()
// {
//     return d->transientContentId;
// }

void Document::addItem(KreenItemPtr item, bool recordUndo)
{
    Q_D();

    qDebug() << "Document::addItem(KreenItemPtr item, bool recordUndo)" << item.get() << recordUndo;
    //d->contentChangedNotificationGroupMethodEnter(recordUndo);

    if (recordUndo) {
        Q_ASSERT(item->id() == -1);
        auto copiedItem = item->deepCopy();
        d->undoStack.push(new AddItemCmd(this, copiedItem)); // this will call addItem with recordUndo=false
        //qDebug() << "(2) d->undoStack.isClean()" << d->undoStack.isClean(); // second because of recursion
        // 1. push calls redo
        // 2. redo call addItem with recordUndo false
        // 3. addItem generates id and sets it in given item
        // 4. we set the id so it is available to the caller
        item->setId(copiedItem->id());
    }
    else {
        if (item->id() == -1) { // only create new id if not in recordUndo mode
            int newId = ++(d->idGenerator);
            item->setId(newId); // caller will see the given id
            item->setZValue(d->maxZValue() + 1);
        }
        d->itemMap.insert(item->id(), item->deepCopy()); // store (add or replace) item including id in the map
        d->itemsCacheDirty = true;
        //qDebug() << "(1) d->undoStack.isClean()" << d->undoStack.isClean();

        // corner case: adding an item on a clean state document:
        //   If the document was clean the receiver of the following call will still receive the clean state.
        //   We compensate for this by connecting the cleanChanged() signal => the receiver will get the correct value.
        //   The concept of the ContentChangedNotificationGroup will not be hurt in the general case.
        d->contentChangedNotificationGroupMethodLeave();
    }
}

void Document::deleteItem(KreenItemPtr item, bool recordUndo)
{
    //d->contentChangedNotificationGroupMethodEnter(recordUndo);
    Q_ASSERT(d->itemMap.contains(item->id()));

    Q_D();

    if (recordUndo) {
        d->undoStack.push(new DeleteItemCmd(this, item));
    }
    else {
        int removedCount = d->itemMap.remove(item->id()); // in Release build we will have a unused-variable warning
        Q_ASSERT(removedCount == 1);
        d->itemsCacheDirty = true;
        d->contentChangedNotificationGroupMethodLeave();
    }
}

/*
 * example: a (lowest)   b   c   d (top-most)
 *
 * with stackBefore (not used):
 *   lower b:         b.stackBefore(item_at(index(b)-1))
 *   raise b:         item_at(index(b)+1).stackBefore(b)
 *   lowerToBottom b: b.stackBefore(item_at(min))
 *   raiseToTop b:    b.stackBefore(item_at(max)); item_at(max).stackBefore(b)
 *
 * with z value:
 *   lower b:         swap z value (a, b)
 *   raise b:         item_at(index(b)+1).stackBefore(b)
 *   lowerToBottom b: b.stackBefore(item_at(min))
 *   raiseToTop b:    b.stackBefore(item_at(max)); item_at(max).stackBefore(b)
 */
void Document::itemStackLowerStep(KreenItemPtr item_in)
{
    Q_ASSERT(d->itemMap.contains(item_in->id()));

    KreenItemPtr prev_item = nullptr;

    foreach (auto item, items()) { // ordered by z
        if (item->id() == item_in->id()) {
            if (prev_item == nullptr) {
                break;
            }
            else {
                contentChangedNotificationGroupBegin(true, "itemStackLower");
                item->setZValue(item->zValue() - 1);
                prev_item->setZValue(prev_item->zValue() + 1);
                applyItemPropertyChanges(item);
                applyItemPropertyChanges(prev_item);
                contentChangedNotificationGroupEnd();
                break;
            }
        }
        prev_item = item;
    }
}

bool Document::hasItemPropertiesChanged(KreenItemPtr item)
{
    Q_ASSERT(d->itemMap.contains(item->id()));
    Q_D();
    auto docItem = d->itemMap[item->id()];
    return !item->deepEquals(docItem);
}

bool Document::applyItemPropertyChanges(KreenItemPtr item, bool recordUndo)
{
    Q_ASSERT(d->itemMap.contains(item->id()));
    Q_D();

    if (!hasItemPropertiesChanged(item)) {
        qDebug() << "[INFO] Document::applyItemPropertyChanges items are equal. Return false";
        return false;
    }

    if (recordUndo) {
        auto oldItem = d->itemMap[item->id()];
        d->undoStack.push(new ApplyItemPropertyChangesCmd(this, item, oldItem));
    }
    else {
        qDebug() << "[DEBUG] Document::applyItemPropertyChanges (itemMap.insert)";
        d->itemMap.insert(item->id(), item->deepCopy());
        d->itemsCacheDirty = true;
        d->contentChangedNotificationGroupMethodLeave();
    }

    return true;
}

void Document::addDemoItems()
{
    contentChangedNotificationGroupBegin(true, "add demo items");

    {
        auto item = KreenItem::create("line");
        item->setLine(QLine(QPoint(10, 20), QPoint(30, 40)));
        addItem(item);
    }

    {
        auto item = KreenItem::create("rect");
        item->setRect(QRect(30, 30, 40, 30));
        item->lineColor()->color = Qt::darkGreen;
        item->lineStyle()->width = 3; // TODO: if this number is uneven, then the item AND the black selection handles become blurred!
        item->lineStyle()->penStyle = Qt::DotLine;
        addItem(item);
    }

    {
        auto item = KreenItem::create("ellipse");
        item->setRect(QRect(10, 40, 40, 40));
        addItem(item);
    }

    {
        auto item = KreenItem::create("ellipse");
        item->setRect(QRect(10, 80, 70, 40));
        item->lineColor()->color = Qt::blue;
        item->lineStyle()->width = 2;
        item->lineStyle()->penStyle = Qt::DashLine;
        addItem(item);
    }

    {
        auto item = KreenItem::create("rect");
        item->setRect(QRect(0, 0, 10, 10));
        item->lineStyle()->width = 1;
        item->lineStyle()->penStyle = Qt::SolidLine;
        item->dropShadow()->enabled = false;
        addItem(item);
    }

    {
        auto item = KreenItem::create("text");
        item->setRect(QRect(10, 120, 200, 40));
        item->lineColor()->color = Qt::gray;
        item->text()->text = "Hello from the document";
        addItem(item);
    }

    {
        auto item = KreenItem::create("text");
        item->setRect(QRect(10, 420, 150, 40));
        item->lineColor()->color = Qt::magenta;
        item->text()->text = "TODO: apply attributes; fill shape; multiline; edit text";
        addItem(item);
    }

    {
        auto item = KreenItem::create("rect");
        item->setRect(QRect(200, 200, 50, 50));
    }

    {
        auto item = KreenItem::create("rect");
        item->setRect(QRect(962-10, 556-10, 10, 10));
        addItem(item);
    }

    contentChangedNotificationGroupEnd();
}

void Document::imageOpCrop(QRect rect)
{
    contentChangedNotificationGroupBegin(true, "Image operation crop");

    setBaseImage(baseImage().copy(rect), true);

    foreach (auto item, items()) {
        item->translate(-rect.x(), -rect.y());
        applyItemPropertyChanges(item); // recordUndo = true
    }

    contentChangedNotificationGroupEnd();
}

const QList<KreenItemPtr> Document::items()
{
    Q_D();

    if (d->itemsCacheDirty) {
        d->itemsCache.clear();

        auto items = d->itemMap.values();
        // sort by z value
        std::sort(items.begin(), items.end(),
                    [](KreenItemPtr a, KreenItemPtr b) { return a->zValue() < b->zValue(); });
        foreach(auto item, items) {
            d->itemsCache.append(item->deepCopy()); // deep copy!
        }

        d->itemsCacheDirty = false;
    }

    return d->itemsCache;
}

QImage Document::renderToImage()
{
    Q_D();

    // not needed:
    // see https://quickmediasolutions.com/blog/14/qeventloop-making-asynchronous-tasks-synchronous
    // see http://developer.nokia.com/community/wiki/How_to_wait_synchronously_for_a_Signal_in_Qt

    d->renderToImageResult = QImage();
    // KreenGraphicsScene::slotRequestRenderToImage will be called directly.
    // But it must be connected (see KreenshotEditor::Impl::createNewDocument)
    emit requestRenderToImageSignal(this);

    // not needed:
//     while (d->renderToImageResult.isNull()) {
//          QThread::currentThread()->sleep(100);
//          qDebug() << "wait 100ms for d->renderToImageResult";
//     }

    // this does not work (KreenGraphicsScene slot will never be called)
//     QEventLoop loop;
//     connect(this, SIGNAL(requestRenderToImageCompleteSignal()), &loop, SLOT(quit()));
//     loop.exec();

    Q_ASSERT(!d->renderToImageResult.isNull());
    return d->renderToImageResult;
}

void Document::onRenderToImageComplete(QImage image)
{
    Q_D();

    d->renderToImageResult = image;
    //emit requestRenderToImageCompleteSignal(); // see renderToImage() (not needed here)
}

void Document::copyImageToClipboard()
{
    QImage image = renderToImage();
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setImage(image);
}

void Document::slotContentChangedSignalEmitter()
{
    emit contentChangedSignal();
}

}
}
