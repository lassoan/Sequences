/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

#ifndef __qSlicerSequenceBrowserModuleWidget_h
#define __qSlicerSequenceBrowserModuleWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

#include "qSlicerSequenceBrowserModuleExport.h"

class qSlicerSequenceBrowserModuleWidgetPrivate;
class vtkMRMLSequenceNode;
class vtkMRMLSequenceBrowserNode;
class vtkMRMLNode;

/// \ingroup Slicer_QtModules_ExtensionTemplate
class Q_SLICER_QTMODULES_SEQUENCEBROWSER_EXPORT qSlicerSequenceBrowserModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT

public:

  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerSequenceBrowserModuleWidget(QWidget *parent=0);
  virtual ~qSlicerSequenceBrowserModuleWidget();

  void setActiveBrowserNode(vtkMRMLSequenceBrowserNode* browserNode);
  void setSequenceRootNode(vtkMRMLSequenceNode* sequenceDataRootNode);

public slots:
  void setMRMLScene(vtkMRMLScene* scene);

  void setSelectedItemNumber(int itemNumber);
  void setPlaybackEnabled(bool play);
  void setPlaybackRateFps(double playbackRateFps);
  void setPlaybackLoopEnabled(bool loopEnabled);  

  void updateChart();

protected slots:
  void activeBrowserNodeChanged(vtkMRMLNode* node);
  void sequenceDataRootNodeChanged(vtkMRMLNode*);
  void onMRMLInputSequenceInputNodeModified(vtkObject* caller);
  void onActiveBrowserNodeModified(vtkObject* caller);
  void onVcrFirst();
  void onVcrPrevious();
  void onVcrNext();
  void onVcrLast();
  void synchronizedRootNodeCheckStateChanged(int aState);

  /// Respond to the scene events
  void onNodeAddedEvent(vtkObject* scene, vtkObject* node);
  void onNodeRemovedEvent(vtkObject* scene, vtkObject* node);
  void onMRMLSceneEndImportEvent();
  void onMRMLSceneEndRestoreEvent();
  void onMRMLSceneEndBatchProcessEvent();
  void onMRMLSceneEndCloseEvent(); 

protected:
  void updateWidgetFromMRML();

  /// Refresh synchronized root nodes table from MRML
  void refreshSynchronizedRootNodesTable();

  QScopedPointer<qSlicerSequenceBrowserModuleWidgetPrivate> d_ptr;
  
  virtual void setup();  
  virtual void enter();
  void exit();

private:
  Q_DECLARE_PRIVATE(qSlicerSequenceBrowserModuleWidget);
  Q_DISABLE_COPY(qSlicerSequenceBrowserModuleWidget);
};

#endif
