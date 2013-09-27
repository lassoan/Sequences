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

// Qt includes
#include <QDebug>
#include <QTimer>

// SlicerQt includes
#include "qSlicerMultiDimensionBrowserModuleWidget.h"
#include "ui_qSlicerMultiDimensionBrowserModuleWidget.h"

// Slicer MRML includes
#include "vtkMRMLHierarchyNode.h"

// MultiDimension includes
#include "vtkSlicerMultiDimensionBrowserLogic.h"
#include "vtkMRMLMultiDimensionBrowserNode.h"

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_MultiDimension
class qSlicerMultiDimensionBrowserModuleWidgetPrivate: public Ui_qSlicerMultiDimensionBrowserModuleWidget
{
  Q_DECLARE_PUBLIC(qSlicerMultiDimensionBrowserModuleWidget);
protected:
  qSlicerMultiDimensionBrowserModuleWidget* const q_ptr;
public:
  qSlicerMultiDimensionBrowserModuleWidgetPrivate(qSlicerMultiDimensionBrowserModuleWidget& object);
  vtkSlicerMultiDimensionBrowserLogic* logic() const;

  /// Using this flag prevents overriding the parameter set node contents when the
  ///   QMRMLCombobox selects the first instance of the specified node type when initializing
  bool ModuleWindowInitialized;

  vtkMRMLMultiDimensionBrowserNode* ActiveBrowserNode;
  QTimer* PlaybackTimer; 
};

//-----------------------------------------------------------------------------
// qSlicerMultiDimensionBrowserModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerMultiDimensionBrowserModuleWidgetPrivate::qSlicerMultiDimensionBrowserModuleWidgetPrivate(qSlicerMultiDimensionBrowserModuleWidget& object)
  : q_ptr(&object)
  , ModuleWindowInitialized(false)
  , ActiveBrowserNode(NULL)
  , PlaybackTimer(NULL)
{
}

//-----------------------------------------------------------------------------
vtkSlicerMultiDimensionBrowserLogic*
qSlicerMultiDimensionBrowserModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerMultiDimensionBrowserModuleWidget);
  
  vtkSlicerMultiDimensionBrowserLogic* logic=vtkSlicerMultiDimensionBrowserLogic::SafeDownCast(q->logic());
  if (logic==NULL)
  {
    qCritical() << "multiDimensionBrowserLogic is invalid";
  }
  return logic;
} 

//-----------------------------------------------------------------------------
// qSlicerMultiDimensionBrowserModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerMultiDimensionBrowserModuleWidget::qSlicerMultiDimensionBrowserModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerMultiDimensionBrowserModuleWidgetPrivate(*this) )
{
  Q_D(qSlicerMultiDimensionBrowserModuleWidget);
  d->PlaybackTimer=new QTimer(this);
  d->PlaybackTimer->setSingleShot(false);
}

//-----------------------------------------------------------------------------
qSlicerMultiDimensionBrowserModuleWidget::~qSlicerMultiDimensionBrowserModuleWidget()
{
}
/*
//-----------------------------------------------------------------------------
void qSlicerMultiDimensionBrowserModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerMultiDimensionBrowserModuleWidget);

  this->Superclass::setMRMLScene(scene);

  qvtkReconnect( d->logic(), scene, vtkMRMLScene::EndImportEvent, this, SLOT(onSceneImportedEvent()) );

  // Find parameters node or create it if there is none in the scene
  if (scene &&  d->logic()->GetMultiDimensionBrowserNode() == 0)
  {
    vtkMRMLNode* node = scene->GetNthNodeByClass(0, "vtkMRMLMultiDimensionBrowserNode");
    if (node)
    {
      this->setMultiDimensionBrowserNode( vtkMRMLMultiDimensionBrowserNode::SafeDownCast(node) );
    }
  }
}
*/
//-----------------------------------------------------------------------------
void qSlicerMultiDimensionBrowserModuleWidget::setup()
{
  Q_D(qSlicerMultiDimensionBrowserModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();
  
  d->MRMLNodeComboBox_MultiDimensionRoot->addAttribute("vtkMRMLHierarchyNode","MultiDimension.NodeType","Root");
  d->MRMLNodeComboBox_VirtualOutput->addAttribute("vtkMRMLHierarchyNode","MultiDimension.NodeType","VirtualOutput");

  connect( d->MRMLNodeComboBox_ActiveBrowser, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(activeBrowserNodeChanged(vtkMRMLNode*)) );
  connect( d->MRMLNodeComboBox_MultiDimensionRoot, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(multiDimensionRootNodeChanged(vtkMRMLNode*)) );
  connect( d->MRMLNodeComboBox_VirtualOutput, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(virtualOutputNodeChanged(vtkMRMLNode*)) );
  connect( d->slider_ParameterValue, SIGNAL(valueChanged(int)), this, SLOT(setParameterValueIndex(int)) );  
  connect( d->pushButton_VcrFirst, SIGNAL(clicked()), this, SLOT(onVcrFirst()) );
  connect( d->pushButton_VcrPrevious, SIGNAL(clicked()), this, SLOT(onVcrPrevious()) );
  connect( d->pushButton_VcrNext, SIGNAL(clicked()), this, SLOT(onVcrNext()) );
  connect( d->pushButton_VcrLast, SIGNAL(clicked()), this, SLOT(onVcrLast()) );
  connect( d->pushButton_VcrPlayPause, SIGNAL(toggled(bool)), this, SLOT(setPlaybackEnabled(bool)) );
  connect( d->pushButton_VcrLoop, SIGNAL(toggled(bool)), this, SLOT(setPlaybackLoopEnabled(bool)) );
  connect( d->doubleSpinBox_VcrPlaybackRate, SIGNAL(valueChanged(double)), this, SLOT(setPlaybackRateFps(double)) );
  d->PlaybackTimer->connect(d->PlaybackTimer, SIGNAL(timeout()),this, SLOT(onVcrNext()));  
}

//-----------------------------------------------------------------------------
void qSlicerMultiDimensionBrowserModuleWidget::enter()
{
  Q_D(qSlicerMultiDimensionBrowserModuleWidget);

  if (this->mrmlScene() != 0)
  {
    // For the user's convenience, create a browser node by default, when entering to the module and no browser node exists in the scene yet
    vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass(0, "vtkMRMLMultiDimensionBrowserNode");
    if (node==NULL)
    {
      vtkSmartPointer<vtkMRMLMultiDimensionBrowserNode> newBrowserNode = vtkSmartPointer<vtkMRMLMultiDimensionBrowserNode>::New();
      this->mrmlScene()->AddNode(newBrowserNode);
      vtkSmartPointer<vtkMRMLHierarchyNode> newVirtualOutputNode = vtkSmartPointer<vtkMRMLHierarchyNode>::New();
      newVirtualOutputNode->SetAttribute("HierarchyType","MultiDimension");
      newVirtualOutputNode->SetAttribute("MultiDimension.NodeType","VirtualOutput");
      this->mrmlScene()->AddNode(newVirtualOutputNode);
      newVirtualOutputNode->SetName(d->MRMLNodeComboBox_VirtualOutput->baseName().toLatin1().constData());
      newBrowserNode->SetAndObserveVirtualOutputNodeID(newVirtualOutputNode->GetID());
      setActiveBrowserNode(newBrowserNode);
    }  
  }
  else
  {
    qCritical() << "Entering the MultiDimensionBrowser module failed, scene is invalid";
  }

  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerMultiDimensionBrowserModuleWidget::activeBrowserNodeChanged(vtkMRMLNode* node)
{
  vtkMRMLMultiDimensionBrowserNode* browserNode = vtkMRMLMultiDimensionBrowserNode::SafeDownCast(node);  
  setActiveBrowserNode(browserNode);
}


//-----------------------------------------------------------------------------
void qSlicerMultiDimensionBrowserModuleWidget::multiDimensionRootNodeChanged(vtkMRMLNode* inputNode)
{
  vtkMRMLHierarchyNode* multiDimensionRootNode = vtkMRMLHierarchyNode::SafeDownCast(inputNode);  
  setMultiDimensionRootNode(multiDimensionRootNode);
}

//-----------------------------------------------------------------------------
void qSlicerMultiDimensionBrowserModuleWidget::virtualOutputNodeChanged(vtkMRMLNode* outputNode)
{
  vtkMRMLHierarchyNode* virtualOutputNode = vtkMRMLHierarchyNode::SafeDownCast(outputNode);  
  setVirtualOutputNode(virtualOutputNode);
}

//-----------------------------------------------------------------------------
void qSlicerMultiDimensionBrowserModuleWidget::onActiveBrowserNodeModified(vtkObject* caller)
{
  updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerMultiDimensionBrowserModuleWidget::onMRMLInputMultiDimensionInputNodeModified(vtkObject* inputNode)
{
  updateWidgetFromMRML();  
}

//-----------------------------------------------------------------------------
void qSlicerMultiDimensionBrowserModuleWidget::onVcrFirst()
{
  Q_D(qSlicerMultiDimensionBrowserModuleWidget);
  d->slider_ParameterValue->setValue(d->slider_ParameterValue->minimum());
}

//-----------------------------------------------------------------------------
void qSlicerMultiDimensionBrowserModuleWidget::onVcrLast()
{
  Q_D(qSlicerMultiDimensionBrowserModuleWidget);
  d->slider_ParameterValue->setValue(d->slider_ParameterValue->maximum());
}

//-----------------------------------------------------------------------------
void qSlicerMultiDimensionBrowserModuleWidget::onVcrPrevious()
{
  Q_D(qSlicerMultiDimensionBrowserModuleWidget);
  int sliderValueToSet=d->slider_ParameterValue->value()-1;
  if (sliderValueToSet>=d->slider_ParameterValue->minimum())
  {
    d->slider_ParameterValue->setValue(sliderValueToSet);
  }
}

//-----------------------------------------------------------------------------
void qSlicerMultiDimensionBrowserModuleWidget::onVcrNext()
{
  Q_D(qSlicerMultiDimensionBrowserModuleWidget);
  int sliderValueToSet=d->slider_ParameterValue->value()+1;
  if (sliderValueToSet<=d->slider_ParameterValue->maximum())
  {
    d->slider_ParameterValue->setValue(sliderValueToSet);
  }
  else
  {
      if (d->ActiveBrowserNode==NULL)
      {
        qCritical() << "onVcrNext failed: no active browser node is selected";
      }
      else
      {
        if (d->ActiveBrowserNode->GetPlaybackLooped())
        {
          onVcrFirst();
        }
        else
        {
          d->ActiveBrowserNode->SetPlaybackActive(false);
          onVcrFirst();
        }
      }
  }
}

//-----------------------------------------------------------------------------
void qSlicerMultiDimensionBrowserModuleWidget::setPlaybackEnabled(bool play)
{
  Q_D(qSlicerMultiDimensionBrowserModuleWidget);
  if (d->ActiveBrowserNode==NULL)
  {
    qCritical() << "onVcrPlayPauseStateChanged failed: no active browser node is selected";
    updateWidgetFromMRML();
    return;
  }
  if (play!=d->ActiveBrowserNode->GetPlaybackActive())
  {
    d->ActiveBrowserNode->SetPlaybackActive(play);
  }
}

//-----------------------------------------------------------------------------
void qSlicerMultiDimensionBrowserModuleWidget::setPlaybackLoopEnabled(bool loopEnabled)
{
  Q_D(qSlicerMultiDimensionBrowserModuleWidget);
  if (d->ActiveBrowserNode==NULL)
  {
    qCritical() << "onVcrPlaybackLoopStateChanged failed: no active browser node is selected";
    updateWidgetFromMRML();
    return;
  }
  if (loopEnabled!=d->ActiveBrowserNode->GetPlaybackLooped())
  {
    d->ActiveBrowserNode->SetPlaybackLooped(loopEnabled);
  }
}

//-----------------------------------------------------------------------------
void qSlicerMultiDimensionBrowserModuleWidget::setPlaybackRateFps(double playbackRateFps)
{
  Q_D(qSlicerMultiDimensionBrowserModuleWidget);
  if (d->ActiveBrowserNode==NULL)
  {
    qCritical() << "setPlaybackRateFps failed: no active browser node is selected";
    updateWidgetFromMRML();
    return;
  }
  if (playbackRateFps!=d->ActiveBrowserNode->GetPlaybackRateFps())
  {
    d->ActiveBrowserNode->SetPlaybackRateFps(playbackRateFps);
  }
}

// --------------------------------------------------------------------------
void qSlicerMultiDimensionBrowserModuleWidget::setActiveBrowserNode(vtkMRMLMultiDimensionBrowserNode* browserNode)
{
  Q_D(qSlicerMultiDimensionBrowserModuleWidget);

  if (d->ActiveBrowserNode!=browserNode)
  { 
    // Reconnect the input node's Modified() event observer
    this->qvtkReconnect(d->ActiveBrowserNode, browserNode, vtkCommand::ModifiedEvent,
      this, SLOT(onActiveBrowserNodeModified(vtkObject*)));
    d->ActiveBrowserNode = browserNode;
  }
  d->MRMLNodeComboBox_ActiveBrowser->setCurrentNode(browserNode);

  updateWidgetFromMRML();
}


// --------------------------------------------------------------------------
void qSlicerMultiDimensionBrowserModuleWidget::setMultiDimensionRootNode(vtkMRMLHierarchyNode* multiDimensionRootNode)
{
  Q_D(qSlicerMultiDimensionBrowserModuleWidget);
  if (d->ActiveBrowserNode==NULL)
  {
    qCritical() << "setMultiDimensionRootNode failed: no active browser node is selected";
    updateWidgetFromMRML();
    return;
  }
  if (multiDimensionRootNode!=d->ActiveBrowserNode->GetRootNode())
  {
    // Reconnect the input node's Modified() event observer
    this->qvtkReconnect(d->ActiveBrowserNode->GetRootNode(), multiDimensionRootNode, vtkCommand::ModifiedEvent,
      this, SLOT(onMRMLInputMultiDimensionInputNodeModified(vtkObject*)));

    char* multiDimensionRootNodeId = multiDimensionRootNode==NULL ? NULL : multiDimensionRootNode->GetID();
    d->ActiveBrowserNode->SetAndObserveRootNodeID(multiDimensionRootNodeId);

    // Update d->ActiveBrowserNode->SetAndObserveSelectedSequenceNodeID
    setParameterValueIndex(0);
  }   
}

// --------------------------------------------------------------------------
void qSlicerMultiDimensionBrowserModuleWidget::setVirtualOutputNode(vtkMRMLHierarchyNode* virtualOutputNode)
{
  Q_D(qSlicerMultiDimensionBrowserModuleWidget);
  if (d->ActiveBrowserNode==NULL)
  {
    qCritical() << "setVirtualOutputNode failed: no active browser node is selected";
    updateWidgetFromMRML();
    return;
  }
  if (virtualOutputNode!=d->ActiveBrowserNode->GetVirtualOutputNode())
  {
    char* virtualOutputNodeId = virtualOutputNode==NULL ? NULL : virtualOutputNode->GetID();
    d->ActiveBrowserNode->SetAndObserveVirtualOutputNodeID(virtualOutputNodeId);
  }  
}

//-----------------------------------------------------------------------------
void qSlicerMultiDimensionBrowserModuleWidget::setParameterValueIndex(int paramValue)
{
  Q_D(qSlicerMultiDimensionBrowserModuleWidget);    
  if (d->ActiveBrowserNode==NULL)
  {
    qCritical() << "setParameterValueIndex failed: no active browser node is selected";
    updateWidgetFromMRML();
    return;
  }
  vtkMRMLHierarchyNode* rootNode=d->ActiveBrowserNode->GetRootNode();
  const char* sequenceNodeId=NULL;
  if (rootNode!=NULL && paramValue>=0)
  {
    if (paramValue<rootNode->GetNumberOfChildrenNodes())
    {
      vtkMRMLHierarchyNode* sequenceNode=rootNode->GetNthChildNode(paramValue);
      if (sequenceNode!=NULL)
      {
        sequenceNodeId=sequenceNode->GetID();
      }
    }
  }
  d->ActiveBrowserNode->SetAndObserveSelectedSequenceNodeID(sequenceNodeId);
}

//-----------------------------------------------------------------------------
void qSlicerMultiDimensionBrowserModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerMultiDimensionBrowserModuleWidget);  
  
  QString DEFAULT_PARAMETER_NAME_STRING=tr("Parameter");  
  
  QObjectList vcrControls;
  vcrControls 
    << d->pushButton_VcrFirst << d->pushButton_VcrLast << d->pushButton_VcrLoop
    << d->pushButton_VcrNext << d->pushButton_VcrPlayPause << d->pushButton_VcrPrevious;
  bool vcrControlsEnabled=false;  

  if (d->ActiveBrowserNode==NULL)
  {
    d->MRMLNodeComboBox_MultiDimensionRoot->setEnabled(false);
    d->MRMLNodeComboBox_VirtualOutput->setEnabled(false);
    d->label_ParameterName->setText(DEFAULT_PARAMETER_NAME_STRING);
    d->label_ParameterUnit->setText("");
    d->slider_ParameterValue->setEnabled(false);
    foreach( QObject*w, vcrControls ) { w->setProperty( "enabled", vcrControlsEnabled ); }
    d->PlaybackTimer->stop();
    return;
  }

  // A valid active browser node is selected
  
  vtkMRMLHierarchyNode* multiDimensionRootNode = d->ActiveBrowserNode->GetRootNode();  
  d->MRMLNodeComboBox_MultiDimensionRoot->setEnabled(true);  
  d->MRMLNodeComboBox_MultiDimensionRoot->setCurrentNode(multiDimensionRootNode);
  d->MRMLNodeComboBox_VirtualOutput->setEnabled(true);

  // Set up the virtual output selector

  vtkMRMLHierarchyNode* virtualOutputNode=d->ActiveBrowserNode->GetVirtualOutputNode();
  d->MRMLNodeComboBox_VirtualOutput->setCurrentNode(virtualOutputNode);

  // Set up the multi-dimension input section (root node selector and sequence slider)

  if (multiDimensionRootNode==NULL)
  {
    d->label_ParameterName->setText(DEFAULT_PARAMETER_NAME_STRING);
    d->label_ParameterUnit->setText("");
    d->slider_ParameterValue->setEnabled(false);
    foreach( QObject*w, vcrControls ) { w->setProperty( "enabled", vcrControlsEnabled ); }
    d->PlaybackTimer->stop();
    return;    
  }

  // A valid multi-dimension root node is selected

  const char* parameterName=multiDimensionRootNode->GetAttribute("MultiDimension.Name");
  if (parameterName!=NULL)
  {
    d->label_ParameterName->setText(parameterName);
  }
  else
  {
    qWarning() << "MultiDimension.Name attribute is not specified in node "<<multiDimensionRootNode->GetID();
    d->label_ParameterName->setText(DEFAULT_PARAMETER_NAME_STRING);
  }

  const char* parameterUnit=multiDimensionRootNode->GetAttribute("MultiDimension.Unit");    
  if (parameterUnit!=NULL)
  {
    d->label_ParameterUnit->setText(parameterUnit);
  }
  else
  {
    qWarning() << "MultiDimension.Unit attribute is not specified in node "<<multiDimensionRootNode->GetID();
    d->label_ParameterUnit->setText("");
  }
  
  int numberOfSequenceNodes=multiDimensionRootNode->GetNumberOfChildrenNodes();
  if (numberOfSequenceNodes>0)
  {
    d->slider_ParameterValue->setEnabled(true);
    d->slider_ParameterValue->setMinimum(0);      
    d->slider_ParameterValue->setMaximum(numberOfSequenceNodes-1);        
    vcrControlsEnabled=true;
    
    bool pushButton_VcrPlayPauseBlockSignals = d->pushButton_VcrPlayPause->blockSignals(true);
    d->pushButton_VcrPlayPause->setChecked(d->ActiveBrowserNode->GetPlaybackActive());
    d->pushButton_VcrPlayPause->blockSignals(pushButton_VcrPlayPauseBlockSignals);

    bool pushButton_VcrLoopBlockSignals = d->pushButton_VcrLoop->blockSignals(true);
    d->pushButton_VcrLoop->setChecked(d->ActiveBrowserNode->GetPlaybackLooped());
    d->pushButton_VcrLoop->blockSignals(pushButton_VcrLoopBlockSignals);
  }
  else
  {
    qDebug() << "Number of child nodes in the selected hierarchy is 0 in node "<<multiDimensionRootNode->GetID();
    d->slider_ParameterValue->setEnabled(false);
  }   

  vtkMRMLHierarchyNode* selectedSequenceNode=d->ActiveBrowserNode->GetSelectedSequenceNode();

  if (selectedSequenceNode!=NULL)
  {
    // TODO: get parameterValueIndex from selectedSequenceNode and update the slider accordingly
    //setParameterValueIndex(parameterValueIndex);
    const char* parameterValue=selectedSequenceNode->GetAttribute("MultiDimension.Value");    
    if (parameterValue!=NULL)
    {
      d->label_ParameterValue->setText(parameterValue);
    }
    else
    {
      qWarning() << "MultiDimension.Value attribute is not specified in node "<<selectedSequenceNode->GetID();
      d->label_ParameterValue->setText("");
    }  
  }
  else
  {
    d->label_ParameterValue->setText("");
    //setParameterValueIndex(0);
  }  
  
  foreach( QObject*w, vcrControls ) { w->setProperty( "enabled", vcrControlsEnabled ); }
  if (vcrControlsEnabled)
  {
    if (d->ActiveBrowserNode->GetPlaybackActive() && d->ActiveBrowserNode->GetPlaybackRateFps()>0)
    {
      int delayInMsec=1000.0/d->ActiveBrowserNode->GetPlaybackRateFps();
      d->PlaybackTimer->start(delayInMsec);
    }
    else
    {
      d->PlaybackTimer->stop();
    }
  }
  else
  {
    d->PlaybackTimer->stop();
  }
}


