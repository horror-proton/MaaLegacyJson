#include "parametereditor.h"
#include "int4edit.h"
#include "qabstractspinbox.h"
#include "qboxlayout.h"
#include "qcheckbox.h"
#include "qgridlayout.h"
#include "qgroupbox.h"
#include "qjsonarray.h"
#include "qjsonobject.h"
#include "qlabel.h"
#include "qlayout.h"
#include "qnamespace.h"
#include "qspinbox.h"
#include "qstackedwidget.h"
#include "qwidget.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QLineEdit>

template <typename C> inline void show_at(C &qobj_container, size_t index) {
  using std::size;
  for (size_t i = 0; i < size(qobj_container); ++i)
    qobj_container[i]->setVisible(i == index);
}

ParameterEditor::ParameterEditor(QWidget *parent) : QWidget(parent) {
  m_node_group = new QGroupBox("Node: \"NULL\"");

  auto recognition_label = new QLabel("Recognition:");
  m_recognition_combo = new QComboBox;
  m_recognition_combo->addItem("None");
  m_recognition_combo->addItem("TemplateMatch");
  m_recognition_combo->addItem("OCR");

  QWidget *recognition_params[3] = {};
  QWidget *action_prarms[3] = {};

  { recognition_params[0] = new QWidget; }
  {
    recognition_params[1] = new QGroupBox;
    auto lo = new QGridLayout;
    recognition_params[1]->setLayout(lo);
    lo->setAlignment(Qt::AlignTop);
    lo->addWidget(new QLabel("Template:"), 0, 0);
    auto template_le = new QLineEdit;
    template_le->setPlaceholderText("(default)");
    lo->addWidget(template_le, 0, 1);
    auto threshold_check = new QCheckBox("Threshold:");
    lo->addWidget(threshold_check, 1, 0);
    auto threshold_slider = new QSlider(Qt::Horizontal);
    threshold_slider->setValue(70);

    connect(threshold_check, &QCheckBox::stateChanged, threshold_slider,
            &QSlider::setEnabled);
    threshold_check->stateChanged(0);

    lo->addWidget(threshold_slider, 1, 1);
    lo->addWidget(new QCheckBox("Chroma key"), 2, 0, 1, 2);
  }
  {
    recognition_params[2] = new QGroupBox;
    auto lo = new QGridLayout;
    recognition_params[2]->setLayout(lo);
    lo->setAlignment(Qt::AlignTop);
    lo->addWidget(new QLabel("Text:"), 0, 0);
    lo->addWidget(new QLineEdit(), 0, 1);
    lo->addWidget(new QLabel("Replace:"), 1, 0);
    lo->addWidget(new QLineEdit(), 1, 1);
  }

  // auto algorithm_groups = new QStackedWidget;

  auto param_layout = new QGridLayout;
  param_layout->setAlignment(Qt::AlignTop);
  int cr = 0;

  param_layout->addWidget(recognition_label, cr, 0, 1, 2);
  ++cr;
  param_layout->addWidget(m_recognition_combo, cr, 0, 1, 2);
  ++cr;

  for (auto p : recognition_params)
    param_layout->addWidget(p, cr, 0, 1, 2);
  ++cr;

  auto roi_label = new QLabel("ROI:");
  m_roi_edit = new Int4Edit;
  param_layout->addWidget(roi_label, cr, 0);
  param_layout->addWidget(m_roi_edit, cr, 1);
  ++cr;

  m_cache_check = new QCheckBox("Cache");
  param_layout->addWidget(m_cache_check, cr, 0, 1, 2);
  ++cr;

  auto action_label = new QLabel("Action:");
  auto action_combo = new QComboBox;
  action_combo->addItem("DoNothing");
  action_combo->addItem("Click");
  action_combo->addItem("Swipe");
  action_combo->addItem("WaitFreezes");
  action_combo->addItem("StartApp");
  action_combo->addItem("StopApp");
  action_combo->addItem("CustomTask");
  param_layout->addWidget(action_label, cr, 0);
  param_layout->addWidget(action_combo, cr, 1);
  ++cr;

  { action_prarms[0] = new QWidget; }
  {
    action_prarms[1] = new QGroupBox;
    auto lo = new QGridLayout;
    action_prarms[1]->setLayout(lo);
    lo->addWidget(new QLabel("Target:"), 0, 0);
    auto target_combo = new QComboBox;
    target_combo->addItem("Default");
    target_combo->addItem("Node");
    target_combo->addItem("Rect");
    lo->addWidget(target_combo, 0, 1);

    QWidget *target_params[3];
    target_params[0] = new QWidget;
    target_params[1] = new QLineEdit;
    target_params[2] = new Int4Edit;
    for (auto p : target_params)
      lo->addWidget(p, 1, 0, 1, 2);
    connect(target_combo, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [=](int index) -> void { show_at(target_params, index); });
    target_combo->currentIndexChanged(0);
  }
  {
    action_prarms[2] = new QGroupBox;
    auto lo = new QGridLayout;
    action_prarms[2]->setLayout(lo);
    auto duration_checkbox = new QCheckBox("Duration:");
    lo->addWidget(duration_checkbox, 0, 0);
    auto duration_sb = new QSpinBox;
    connect(duration_checkbox, &QCheckBox::stateChanged, duration_sb,
            &QSpinBox::setEnabled);
    duration_checkbox->stateChanged(0);
    lo->addWidget(duration_sb, 0, 1);
  }
  for (auto p : action_prarms)
    param_layout->addWidget(p, cr, 0, 1, 2);
  ++cr;

  m_node_group->setLayout(param_layout);

  auto root_layout = new QGridLayout;
  root_layout->addWidget(m_node_group, 0, 0, 0, 0);
  setLayout(root_layout);

  connect(m_recognition_combo, qOverload<int>(&QComboBox::currentIndexChanged),
          this, [=](int index) -> void { show_at(recognition_params, index); });

  connect(action_combo, qOverload<int>(&QComboBox::currentIndexChanged), this,
          [=](int index) -> void { show_at(action_prarms, index); });

  m_recognition_combo->currentIndexChanged(0);
  action_combo->currentIndexChanged(0);
}

void ParameterEditor::load_from_json(const QJsonObject &json) {
  {
    auto jt = json.value("recognition").toString();
    if (jt == "TemplateMatch") {
      m_recognition_combo->setCurrentIndex(1);
    } else if (jt == "OCR") {
      m_recognition_combo->setCurrentIndex(2);
    } else {
      m_recognition_combo->setCurrentIndex(0);
    }
    // TODO: remove other keys
  }
  {
    auto jroi = json.value("roi").toArray();
    m_roi_edit->set_value(jroi);
  }
  {
    auto jcache = json.value("cache").toBool();
    if (jcache)
      m_cache_check->setChecked(true);
  }
  // TODO
}

void ParameterEditor::store_to_json(QJsonObject &json) {
  // TODO
}

void ParameterEditor::update_selection(Node *node) {
  if (m_node == node)
    return;
  if (m_node)
    store_to_json(m_node->m_node_info);
  if (node) {
    load_from_json(node->m_node_info);
    m_node_group->setTitle("Node: " + node->m_label);
  } else {
    m_node_group->setTitle("Node: NULL");
  }
  m_node = node;
}