#include "networkwidget.h"
#include "node.h"
#include "nodeslotin.h"
#include "nodeslotout.h"
#include "qgraphicsscene.h"
#include "qnamespace.h"
#include <QJsonArray>
#include <QJsonObject>

#include "QDebug"
#include "QMouseEvent"
#include "wire.h"

NetworkWidget::NetworkWidget(QWidget *parent) : QGraphicsView(parent) {
  {
    auto scene = new QGraphicsScene(this);
    scene->setItemIndexMethod(QGraphicsScene::NoIndex);
    setScene(scene);
  }

  setTransformationAnchor(AnchorUnderMouse);
  setSceneRect({});
  setCacheMode(CacheBackground);
  setRenderHints(QPainter::Antialiasing);

  scale(1.5, 1.5);
  /*
    auto n1 = new Node(this);
    n1->setPos(100, 100);
    scene()->addItem(n1);

    auto n2 = new Node(this);
    n2->setPos(200, 200);
    scene()->addItem(n2);

    auto n3 = new Node(this);
    n3->setPos(-100, -100);
    scene()->addItem(n3);
  */
}

void NetworkWidget::import_json(const QJsonObject &root) {
  double y = 0;
  double x = 0;
  for (auto k : root.keys()) {
    auto node = new Node(this);
    node->setPos(x, y);
    node->m_label = k;
    scene()->addItem(node);
    m_node_key_map.insert_or_assign(k.toStdString(), node);
    if (x < 2000)
      x += 150;
    else
      x = 0, y += 150;
  }

  for (auto k : root.keys()) {
    Node *node = m_node_key_map[k.toStdString()];
    auto nextarr = root[k]["next"].toArray();
    for (auto other_key : nextarr) {
      auto iter = m_node_key_map.find(other_key.toString().toStdString());
      if (iter == m_node_key_map.end()) {
        qDebug() << "node " << other_key << " not found";
        // TODO: connect to a textbox representing unresolved nodes
        continue;
      }
      auto dst_node = iter->second;
      auto src_slot = node->add_out_slot();
      auto dst_slot = dst_node->in_slot();
      createWire(src_slot, dst_slot);
    }
    node->regenerate_reserved_out_slots();
  }
}

void NetworkWidget::drawBackground(QPainter *painter, const QRectF &rect) {
  painter->setPen(Qt::NoPen);
  painter->fillRect(rect, Qt::gray);
  painter->drawRect(sceneRect());
}

void NetworkWidget::mouseMoveEvent(QMouseEvent *event) {
  if (m_pending_wire) {
    auto scene_pos = mapToScene(event->pos());
    if (m_virt_dst) {
      m_virt_dst->setPos(scene_pos);
      m_pending_wire->adjust();
    }
    if (m_virt_src) {
      m_virt_src->setPos(scene_pos);
      m_pending_wire->adjust();
    }
  }
  QGraphicsView::mouseMoveEvent(event);
}

void NetworkWidget::mousePressEvent(QMouseEvent *event) {
  QGraphicsView::mousePressEvent(event);
}

void NetworkWidget::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::RightButton) {
    clearPendingWire();
  }
  QGraphicsView::mouseReleaseEvent(event);
}

void NetworkWidget::addSrcToPendingWire(NodeSlotOut *src) {
  clearPendingWire();
  m_virt_dst = new NodeSlotIn(nullptr);
  m_virt_dst->setPos(src->mapToScene(0, 0));
  scene()->addItem(m_virt_dst);
  m_pending_wire = new Wire(this, src, m_virt_dst);
  scene()->addItem(m_pending_wire);

  // setMouseTracking(true);
}

void NetworkWidget::addPendingToDstWire(NodeSlotIn *dst) {
  clearPendingWire();
  m_virt_src = new NodeSlotOut(nullptr);
  m_virt_src->setPos(dst->mapToScene(0, 0));
  scene()->addItem(m_virt_src);
  m_pending_wire = new Wire(this, m_virt_src, dst);
  scene()->addItem(m_pending_wire);
}

void NetworkWidget::clearPendingWire() {
  if (m_pending_wire) {
    scene()->removeItem(m_pending_wire);
    delete m_pending_wire;
    m_pending_wire = nullptr;
  }
  if (m_virt_src) {
    scene()->removeItem(m_virt_src);
    delete m_virt_src;
    m_virt_src = nullptr;
  }
  if (m_virt_dst) {
    scene()->removeItem(m_virt_dst);
    delete m_virt_dst;
    m_virt_dst = nullptr;
  }
}

void NetworkWidget::createWire(NodeSlotOut *src, NodeSlotIn *dst) {
  auto wire = new Wire(this, src, dst);
  dst->m_wires.append(wire);
  src->m_wires.append(wire);
  scene()->addItem(wire);
  clearPendingWire();
}

void NetworkWidget::deleteWire(Wire *wire) {
  wire->m_src_slot->m_wires.removeOne(wire);
  wire->m_dst_slot->m_wires.removeOne(wire);
  scene()->removeItem(wire);
  delete wire;
}