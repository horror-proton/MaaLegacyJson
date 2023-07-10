#include "nodeslotout.h"

#include "node.h"
#include "qgraphicssceneevent.h"
#include "qnamespace.h"
#include "wire.h"
#include <QDebug>
#include <QGraphicsItem>

NodeSlotOut::NodeSlotOut(Node *parent, bool reserved)
    : m_parent_node(parent), m_reserved(reserved) {
  if (parent) {
    setParentItem(static_cast<QGraphicsItem *>(parent));
    setAcceptedMouseButtons(Qt::LeftButton);
  } else {
    setAcceptedMouseButtons(Qt::NoButton);
  }
  setFlag(ItemSendsGeometryChanges);
  setCacheMode(DeviceCoordinateCache);
}

void NodeSlotOut::paint(QPainter *painter,
                        const QStyleOptionGraphicsItem *option,
                        QWidget *widget) {
  if (isOrphan())
    return;
  if (!isReservedNode()) {
    painter->setPen(Qt::NoPen);
    painter->setBrush(Qt::white);
  } else {
    painter->setPen(Qt::white);
    painter->setBrush(Qt::NoBrush);
  }
  painter->drawEllipse(-3, -3, 6, 6);
}

QRectF NodeSlotOut::boundingRect() const { return QRectF{-4, -4, 8, 8}; }

void NodeSlotOut::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
  QGraphicsItem::mouseReleaseEvent(event);

  auto &network = *m_parent_node->parent_network();
  if (isReservedNode()) {
    auto pending_wire = network.m_pending_wire;
    if (pending_wire && pending_wire->m_src_slot->isOrphan()) {
      auto dst_slot = pending_wire->m_dst_slot;
      auto src_slot = m_parent_node->add_out_slot(nodeIndex());
      network.createWire(src_slot, dst_slot);
      m_parent_node
          ->regenerate_reserved_out_slots(); // FIXME: causing delete this;?
    } else
      network.addSrcToPendingWire(this);
  } else if (!m_wires.isEmpty()) {
    m_parent_node->out_slot_disconnect(this); // FIXME: causing delete this;
  }
}

void NodeSlotOut::mousePressEvent(QGraphicsSceneMouseEvent *event) {
  QGraphicsItem::mousePressEvent(event);
  event->accept();
}

int NodeSlotOut::nodeIndex() const {
  if (isOrphan())
    return -1;
  if (isReservedNode())
    return m_parent_node->m_reserved_out_slots.indexOf(
        const_cast<NodeSlotOut *>(this));
  else
    return m_parent_node->m_out_slots.indexOf(const_cast<NodeSlotOut *>(this));
}
