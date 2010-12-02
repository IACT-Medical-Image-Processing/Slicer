/*==============================================================================

  Program: 3D Slicer

  Copyright (c) 2010 Kitware Inc.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Julien Finet, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// Qt includes
#include <QDebug>
#include <QMap>
#include <QMimeData>
#include <QSharedPointer>
#include <QStack>
#include <QStringList>
#include <QVector>

// qMRML includes
#include "qMRMLSceneAnnotationModel.h"
#include "qMRMLSceneDisplayableModel.h"
//#include "qMRMLUtils.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLNode.h>
#include <vtkMRMLAnnotationNode.h>
#include <vtkMRMLDisplayableHierarchyNode.h>

// VTK includes
#include <vtkVariantArray.h>
#include <typeinfo>

//------------------------------------------------------------------------------
qMRMLSceneAnnotationModel::qMRMLSceneAnnotationModel(QObject *vparent)
  :qMRMLSceneDisplayableModel(vparent)
{
  this->setListenNodeModifiedEvent(true);
}

//------------------------------------------------------------------------------
qMRMLSceneAnnotationModel::~qMRMLSceneAnnotationModel()
{
}

//------------------------------------------------------------------------------
void qMRMLSceneAnnotationModel::updateNodeFromItem(vtkMRMLNode* node, QStandardItem* item)
{
  this->qMRMLSceneDisplayableModel::updateNodeFromItem(node,item);

  //this->m_Widget->refreshTree();
}

//------------------------------------------------------------------------------
void qMRMLSceneAnnotationModel::updateItemFromNode(QStandardItem* item, vtkMRMLNode* node, int column)
{

  // from qMRMLSceneModel
  bool oldBlock = this->blockSignals(true);
  item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
  item->setData(QString(node->GetID()), qMRML::UIDRole);
  item->setData(QVariant::fromValue(reinterpret_cast<long long>(node)), qMRML::PointerRole);
  this->blockSignals(oldBlock);

  vtkMRMLAnnotationNode* annotationNode = vtkMRMLAnnotationNode::SafeDownCast(node);

  switch (column)
    {
    case qMRMLSceneAnnotationModel::VisibilityColumn:
      // the visibility icon
      this->blockSignals(true);
      item->setFlags(item->flags() | Qt::ItemIsSelectable);
      this->blockSignals(oldBlock);

      if (annotationNode)
        {
        if (annotationNode->GetVisible())
          {
          item->setData(QPixmap(":/Icons/AnnotationVisibility.png"),Qt::DecorationRole);
          }
        else
          {
          item->setData(QPixmap(":/Icons/AnnotationInvisible.png"),Qt::DecorationRole);
          }
        break;
        }
      // TODO for hierarchies..
      item->setData(QPixmap(":/Icons/AnnotationVisibility.png"),Qt::DecorationRole);
      break;
    case qMRMLSceneAnnotationModel::LockColumn:
      // the lock/unlock icon
      this->blockSignals(true);
      item->setFlags(item->flags() | Qt::ItemIsSelectable);
      this->blockSignals(oldBlock);

      if (annotationNode)
        {
        if (annotationNode->GetLocked())
          {
          item->setData(QPixmap(":/Icons/AnnotationLock.png"),Qt::DecorationRole);
          }
        else
          {
          item->setData(QPixmap(":/Icons/AnnotationUnlock.png"),Qt::DecorationRole);
          }
        break;
        }
      // TODO for hierarchies..
      item->setData(QPixmap(":/Icons/AnnotationUnlock.png"),Qt::DecorationRole);
      break;
    case qMRMLSceneAnnotationModel::EditColumn:
        // the annotation type icon
        this->blockSignals(true);
        item->setFlags(item->flags() | Qt::ItemIsSelectable);
        this->blockSignals(oldBlock);
        item->setData(QPixmap(this->m_Logic->GetAnnotationIcon(node->GetID())),Qt::DecorationRole);
        break;
    case qMRMLSceneAnnotationModel::ValueColumn:
      if (annotationNode)
        {
        // the annotation measurement
        this->blockSignals(true);
        item->setFlags(item->flags() | Qt::ItemIsSelectable);
        this->blockSignals(oldBlock);
        item->setText(QString(this->m_Logic->GetAnnotationMeasurement(annotationNode->GetID(),false)));
        break;
        }
      else if (node->IsA("vtkMRMLAnnotationHierarchyNode"))
        {
        this->blockSignals(true);
        item->setFlags(item->flags() | Qt::ItemIsSelectable);
        this->blockSignals(oldBlock);
        item->setText(QString(node->GetName()));
        break;
        }
    case qMRMLSceneAnnotationModel::TextColumn:
      if (annotationNode)
        {
        // the annotation text
        this->blockSignals(true);
        item->setFlags(item->flags() | Qt::ItemIsEditable | Qt::ItemIsSelectable);
        this->blockSignals(oldBlock);
        item->setText(QString(this->m_Logic->GetAnnotationText(annotationNode->GetID())));
        break;
        }
    }


  // from qMRMLSceneDisplayableModel
  this->blockSignals(true);
  if (qMRMLSceneDisplayableModel::canBeAChild(node))
    {
    item->setFlags(item->flags() | Qt::ItemIsDragEnabled);
    }
  if (qMRMLSceneDisplayableModel::canBeAParent(node))
    {
    item->setFlags(item->flags() | Qt::ItemIsDropEnabled);
    }
  this->blockSignals(oldBlock);
  QStandardItem* parentItem = item->parent();
  QStandardItem* newParentItem = this->itemFromNode(qMRMLSceneDisplayableModel::parentNode(node));
  if (newParentItem == 0)
    {
    newParentItem = this->mrmlSceneItem();
    }
  // if the item has no parent, then it means it hasn't been put into the scene yet.
  // and it will do it automatically.
  if (parentItem != 0 && (parentItem != newParentItem || qMRMLSceneDisplayableModel::nodeIndex(node) != item->row()))
    {
    QList<QStandardItem*> children = parentItem->takeRow(item->row());
    int min = this->preItems(newParentItem).count();
    int max = newParentItem->rowCount() - this->postItems(newParentItem).count();
    int pos = qMin(min + qMRMLSceneDisplayableModel::nodeIndex(node), max);
    newParentItem->insertRow(pos, children);
    }

}

//------------------------------------------------------------------------------
int qMRMLSceneAnnotationModel::columnCount(const QModelIndex &_parent)const
{
  Q_UNUSED(_parent);
  return 6;
}

//------------------------------------------------------------------------------
QVariant qMRMLSceneAnnotationModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole)
    {
        if (orientation == Qt::Horizontal) {
            switch (section)
            {
            case 0:
                return QString("");
            case 1:
                return QString("Vis");
            case 2:
                return QString("Lock");
            case 3:
                return QString("Edit");
            case 4:
                return QString("Value");
            case 5:
                return QString("Text");
            }
        }
    }
    return QVariant();
}

//-----------------------------------------------------------------------------
/// Set and observe the GUI widget
//-----------------------------------------------------------------------------
void qMRMLSceneAnnotationModel::setAndObserveWidget(qSlicerAnnotationModuleWidget* widget)
{
  if (!widget)
    {
    return;
    }

  this->m_Widget = widget;

}

//-----------------------------------------------------------------------------
/// Set and observe the logic
//-----------------------------------------------------------------------------
void qMRMLSceneAnnotationModel::setAndObserveLogic(vtkSlicerAnnotationModuleLogic* logic)
{
  if (!logic)
    {
    return;
    }

  this->m_Logic = logic;

}
