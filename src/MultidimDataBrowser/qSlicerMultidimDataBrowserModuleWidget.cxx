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
#include "qSlicerMultidimDataBrowserModuleWidget.h"
#include "ui_qSlicerMultidimDataBrowserModuleWidget.h"

// MRML includes
#include "vtkMRMLScene.h"

// MultidimData includes
#include "vtkSlicerMultidimDataBrowserLogic.h"
#include "vtkMRMLMultidimDataBrowserNode.h"
#include "vtkMRMLMultidimDataNode.h"

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_MultidimData
class qSlicerMultidimDataBrowserModuleWidgetPrivate: public Ui_qSlicerMultidimDataBrowserModuleWidget
{
  Q_DECLARE_PUBLIC(qSlicerMultidimDataBrowserModuleWidget);
protected:
  qSlicerMultidimDataBrowserModuleWidget* const q_ptr;
public:
  qSlicerMultidimDataBrowserModuleWidgetPrivate(qSlicerMultidimDataBrowserModuleWidget& object);
  vtkSlicerMultidimDataBrowserLogic* logic() const;
  vtkMRMLMultidimDataBrowserNode* activeBrowserNode() const;

  /// Using this flag prevents overriding the parameter set node contents when the
  ///   QMRMLCombobox selects the first instance of the specified node type when initializing
  bool ModuleWindowInitialized;

  std::string ActiveBrowserNodeID;
  QTimer* PlaybackTimer; 
};

//-----------------------------------------------------------------------------
// qSlicerMultidimDataBrowserModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerMultidimDataBrowserModuleWidgetPrivate::qSlicerMultidimDataBrowserModuleWidgetPrivate(qSlicerMultidimDataBrowserModuleWidget& object)
  : q_ptr(&object)
  , ModuleWindowInitialized(false)
  , PlaybackTimer(NULL)
{
}

//-----------------------------------------------------------------------------
vtkMRMLMultidimDataBrowserNode* qSlicerMultidimDataBrowserModuleWidgetPrivate::activeBrowserNode() const
{
  Q_Q(const qSlicerMultidimDataBrowserModuleWidget);
  if (ActiveBrowserNodeID.empty())
  {
    return NULL;
  }
  vtkMRMLScene* scene=q->mrmlScene();
  if (scene==0)
  {
    return NULL;
  }
  vtkMRMLMultidimDataBrowserNode* node=vtkMRMLMultidimDataBrowserNode::SafeDownCast(scene->GetNodeByID(ActiveBrowserNodeID.c_str()));
  return node;
}

//-----------------------------------------------------------------------------
vtkSlicerMultidimDataBrowserLogic*
qSlicerMultidimDataBrowserModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerMultidimDataBrowserModuleWidget);
  
  vtkSlicerMultidimDataBrowserLogic* logic=vtkSlicerMultidimDataBrowserLogic::SafeDownCast(q->logic());
  if (logic==NULL)
  {
    qCritical() << "multiDimensionBrowserLogic is invalid";
  }
  return logic;
} 

//-----------------------------------------------------------------------------
// qSlicerMultidimDataBrowserModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerMultidimDataBrowserModuleWidget::qSlicerMultidimDataBrowserModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerMultidimDataBrowserModuleWidgetPrivate(*this) )
{
  Q_D(qSlicerMultidimDataBrowserModuleWidget);
  d->PlaybackTimer=new QTimer(this);
  d->PlaybackTimer->setSingleShot(false);
}

//-----------------------------------------------------------------------------
qSlicerMultidimDataBrowserModuleWidget::~qSlicerMultidimDataBrowserModuleWidget()
{
}
/*
//-----------------------------------------------------------------------------
void qSlicerMultidimDataBrowserModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerMultidimDataBrowserModuleWidget);

  this->Superclass::setMRMLScene(scene);

  qvtkReconnect( d->logic(), scene, vtkMRMLScene::EndImportEvent, this, SLOT(onSceneImportedEvent()) );

  // Find parameters node or create it if there is none in the scene
  if (scene &&  d->logic()->GetMultidimDataBrowserNode() == 0)
  {
    vtkMRMLNode* node = scene->GetNthNodeByClass(0, "vtkMRMLMultidimDataBrowserNode");
    if (node)
    {
      this->setMultidimDataBrowserNode( vtkMRMLMultidimDataBrowserNode::SafeDownCast(node) );
    }
  }
}
*/
//-----------------------------------------------------------------------------
void qSlicerMultidimDataBrowserModuleWidget::setup()
{
  Q_D(qSlicerMultidimDataBrowserModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();
  
  connect( d->MRMLNodeComboBox_ActiveBrowser, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(activeBrowserNodeChanged(vtkMRMLNode*)) );
  connect( d->MRMLNodeComboBox_MultidimDataRoot, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(multidimDataRootNodeChanged(vtkMRMLNode*)) );
  connect( d->slider_ParameterValue, SIGNAL(valueChanged(int)), this, SLOT(setSelectedBundleIndex(int)) );  
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
void qSlicerMultidimDataBrowserModuleWidget::enter()
{
  Q_D(qSlicerMultidimDataBrowserModuleWidget);

  if (this->mrmlScene() != 0)
  {
    // For the user's convenience, create a browser node by default, when entering to the module and no browser node exists in the scene yet
    vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass(0, "vtkMRMLMultidimDataBrowserNode");
    if (node==NULL)
    {
      vtkSmartPointer<vtkMRMLMultidimDataBrowserNode> newBrowserNode = vtkSmartPointer<vtkMRMLMultidimDataBrowserNode>::New();
      this->mrmlScene()->AddNode(newBrowserNode);      
      setActiveBrowserNode(newBrowserNode);
    }
  }
  else
  {
    qCritical() << "Entering the MultidimDataBrowser module failed, scene is invalid";
  }

  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerMultidimDataBrowserModuleWidget::activeBrowserNodeChanged(vtkMRMLNode* node)
{
  vtkMRMLMultidimDataBrowserNode* browserNode = vtkMRMLMultidimDataBrowserNode::SafeDownCast(node);  
  setActiveBrowserNode(browserNode);
}


//-----------------------------------------------------------------------------
void qSlicerMultidimDataBrowserModuleWidget::multidimDataRootNodeChanged(vtkMRMLNode* inputNode)
{
  vtkMRMLMultidimDataNode* multidimDataRootNode = vtkMRMLMultidimDataNode::SafeDownCast(inputNode);  
  setMultidimDataRootNode(multidimDataRootNode);
}

//-----------------------------------------------------------------------------
void qSlicerMultidimDataBrowserModuleWidget::onActiveBrowserNodeModified(vtkObject* caller)
{
  updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerMultidimDataBrowserModuleWidget::onMRMLInputMultidimDataInputNodeModified(vtkObject* inputNode)
{
  updateWidgetFromMRML();  
}

//-----------------------------------------------------------------------------
void qSlicerMultidimDataBrowserModuleWidget::onVcrFirst()
{
  Q_D(qSlicerMultidimDataBrowserModuleWidget);
  d->slider_ParameterValue->setValue(d->slider_ParameterValue->minimum());
}

//-----------------------------------------------------------------------------
void qSlicerMultidimDataBrowserModuleWidget::onVcrLast()
{
  Q_D(qSlicerMultidimDataBrowserModuleWidget);
  d->slider_ParameterValue->setValue(d->slider_ParameterValue->maximum());
}

//-----------------------------------------------------------------------------
void qSlicerMultidimDataBrowserModuleWidget::onVcrPrevious()
{
  Q_D(qSlicerMultidimDataBrowserModuleWidget);
  int sliderValueToSet=d->slider_ParameterValue->value()-1;
  if (sliderValueToSet>=d->slider_ParameterValue->minimum())
  {
    d->slider_ParameterValue->setValue(sliderValueToSet);
  }
}

//-----------------------------------------------------------------------------
void qSlicerMultidimDataBrowserModuleWidget::onVcrNext()
{
  Q_D(qSlicerMultidimDataBrowserModuleWidget);
  int sliderValueToSet=d->slider_ParameterValue->value()+1;
  if (sliderValueToSet<=d->slider_ParameterValue->maximum())
  {
    d->slider_ParameterValue->setValue(sliderValueToSet);
  }
  else
  {
      if (d->activeBrowserNode()==NULL)
      {
        qCritical() << "onVcrNext failed: no active browser node is selected";
      }
      else
      {
        if (d->activeBrowserNode()->GetPlaybackLooped())
        {
          onVcrFirst();
        }
        else
        {
          d->activeBrowserNode()->SetPlaybackActive(false);
          onVcrFirst();
        }
      }
  }
}

//-----------------------------------------------------------------------------
void qSlicerMultidimDataBrowserModuleWidget::setPlaybackEnabled(bool play)
{
  Q_D(qSlicerMultidimDataBrowserModuleWidget);
  if (d->activeBrowserNode()==NULL)
  {
    qCritical() << "onVcrPlayPauseStateChanged failed: no active browser node is selected";
    updateWidgetFromMRML();
    return;
  }
  if (play!=d->activeBrowserNode()->GetPlaybackActive())
  {
    d->activeBrowserNode()->SetPlaybackActive(play);
  }
}

//-----------------------------------------------------------------------------
void qSlicerMultidimDataBrowserModuleWidget::setPlaybackLoopEnabled(bool loopEnabled)
{
  Q_D(qSlicerMultidimDataBrowserModuleWidget);
  if (d->activeBrowserNode()==NULL)
  {
    qCritical() << "onVcrPlaybackLoopStateChanged failed: no active browser node is selected";
    updateWidgetFromMRML();
    return;
  }
  if (loopEnabled!=d->activeBrowserNode()->GetPlaybackLooped())
  {
    d->activeBrowserNode()->SetPlaybackLooped(loopEnabled);
  }
}

//-----------------------------------------------------------------------------
void qSlicerMultidimDataBrowserModuleWidget::setPlaybackRateFps(double playbackRateFps)
{
  Q_D(qSlicerMultidimDataBrowserModuleWidget);
  if (d->activeBrowserNode()==NULL)
  {
    qCritical() << "setPlaybackRateFps failed: no active browser node is selected";
    updateWidgetFromMRML();
    return;
  }
  if (playbackRateFps!=d->activeBrowserNode()->GetPlaybackRateFps())
  {
    d->activeBrowserNode()->SetPlaybackRateFps(playbackRateFps);
  }
}

// --------------------------------------------------------------------------
void qSlicerMultidimDataBrowserModuleWidget::setActiveBrowserNode(vtkMRMLMultidimDataBrowserNode* browserNode)
{
  Q_D(qSlicerMultidimDataBrowserModuleWidget);

  if (d->activeBrowserNode()!=browserNode)
  { 
    // Reconnect the input node's Modified() event observer
    this->qvtkReconnect(d->activeBrowserNode(), browserNode, vtkCommand::ModifiedEvent,
      this, SLOT(onActiveBrowserNodeModified(vtkObject*)));
    d->ActiveBrowserNodeID = (browserNode?browserNode->GetID():"");
  }
  d->MRMLNodeComboBox_ActiveBrowser->setCurrentNode(browserNode);

  updateWidgetFromMRML();
}


// --------------------------------------------------------------------------
void qSlicerMultidimDataBrowserModuleWidget::setMultidimDataRootNode(vtkMRMLMultidimDataNode* multidimDataRootNode)
{
  Q_D(qSlicerMultidimDataBrowserModuleWidget);
  if (d->activeBrowserNode()==NULL)
  {
    qCritical() << "setMultidimDataRootNode failed: no active browser node is selected";
    updateWidgetFromMRML();
    return;
  }
  if (multidimDataRootNode!=d->activeBrowserNode()->GetRootNode())
  {
    // Reconnect the input node's Modified() event observer
    this->qvtkReconnect(d->activeBrowserNode()->GetRootNode(), multidimDataRootNode, vtkCommand::ModifiedEvent,
      this, SLOT(onMRMLInputMultidimDataInputNodeModified(vtkObject*)));

    char* multidimDataRootNodeId = multidimDataRootNode==NULL ? NULL : multidimDataRootNode->GetID();
    d->activeBrowserNode()->SetAndObserveRootNodeID(multidimDataRootNodeId);

    // Update d->activeBrowserNode()->SetAndObserveSelectedSequenceNodeID
    setSelectedBundleIndex(0);
  }   
}

//-----------------------------------------------------------------------------
void qSlicerMultidimDataBrowserModuleWidget::setSelectedBundleIndex(int bundleIndex)
{
  /* TODO
  Q_D(qSlicerMultidimDataBrowserModuleWidget);    
  if (d->activeBrowserNode()==NULL)
  {
    qCritical() << "setSelectedBundleIndex failed: no active browser node is selected";
    updateWidgetFromMRML();
    return;
  }
  int selectedBundleIndex=-1;
  vtkMRMLMultidimDataNode* rootNode=d->activeBrowserNode()->GetRootNode();  
  if (rootNode!=NULL && bundleIndex>=0)
  {
    if (bundleIndex<rootNode->GetNumberOfBundles())
    {
      selectedBundleIndex=bundleIndex;
    }
  }
  d->activeBrowserNode()->SetSelectedBundleIndex(selectedBundleIndex);
  */
}

//-----------------------------------------------------------------------------
void qSlicerMultidimDataBrowserModuleWidget::updateWidgetFromMRML()
{
  /* TODO
  Q_D(qSlicerMultidimDataBrowserModuleWidget);
  
  QString DEFAULT_PARAMETER_NAME_STRING=tr("Parameter");  
  
  QObjectList vcrControls;
  vcrControls 
    << d->pushButton_VcrFirst << d->pushButton_VcrLast << d->pushButton_VcrLoop
    << d->pushButton_VcrNext << d->pushButton_VcrPlayPause << d->pushButton_VcrPrevious;
  bool vcrControlsEnabled=false;  

  if (d->activeBrowserNode()==NULL)
  {
    d->MRMLNodeComboBox_MultidimDataRoot->setEnabled(false);
    d->label_ParameterName->setText(DEFAULT_PARAMETER_NAME_STRING);
    d->label_ParameterUnit->setText("");
    d->slider_ParameterValue->setEnabled(false);
    foreach( QObject*w, vcrControls ) { w->setProperty( "enabled", vcrControlsEnabled ); }
    d->PlaybackTimer->stop();
    return;
  }

  // A valid active browser node is selected
  
  vtkMRMLMultidimDataNode* multidimDataRootNode = d->activeBrowserNode()->GetRootNode();  
  d->MRMLNodeComboBox_MultidimDataRoot->setEnabled(true);  
  d->MRMLNodeComboBox_MultidimDataRoot->setCurrentNode(multidimDataRootNode);

  // Set up the multidimensional input section (root node selector and sequence slider)

  if (multidimDataRootNode==NULL)
  {
    d->label_ParameterName->setText(DEFAULT_PARAMETER_NAME_STRING);
    d->label_ParameterUnit->setText("");
    d->slider_ParameterValue->setEnabled(false);
    foreach( QObject*w, vcrControls ) { w->setProperty( "enabled", vcrControlsEnabled ); }
    d->PlaybackTimer->stop();
    return;    
  }

  // A valid multidimensional root node is selected

  const char* parameterName=multidimDataRootNode->GetDimensionName();
  if (parameterName!=NULL)
  {
    d->label_ParameterName->setText(parameterName);
  }
  else
  {
    qWarning() << "Dimension name is not specified in node "<<multidimDataRootNode->GetID();
    d->label_ParameterName->setText(DEFAULT_PARAMETER_NAME_STRING);
  }

  const char* parameterUnit=multidimDataRootNode->GetUnit();
  if (parameterUnit!=NULL)
  {
    d->label_ParameterUnit->setText(parameterUnit);
  }
  else
  {
    qWarning() << "Unit is not specified in node "<<multidimDataRootNode->GetID();
    d->label_ParameterUnit->setText("");
  }
  
  int numberOfBundles=multidimDataRootNode->GetNumberOfBundles();
  if (numberOfBundles>0)
  {
    d->slider_ParameterValue->setEnabled(true);
    d->slider_ParameterValue->setMinimum(0);      
    d->slider_ParameterValue->setMaximum(numberOfBundles-1);        
    vcrControlsEnabled=true;
    
    bool pushButton_VcrPlayPauseBlockSignals = d->pushButton_VcrPlayPause->blockSignals(true);
    d->pushButton_VcrPlayPause->setChecked(d->activeBrowserNode()->GetPlaybackActive());
    d->pushButton_VcrPlayPause->blockSignals(pushButton_VcrPlayPauseBlockSignals);

    bool pushButton_VcrLoopBlockSignals = d->pushButton_VcrLoop->blockSignals(true);
    d->pushButton_VcrLoop->setChecked(d->activeBrowserNode()->GetPlaybackLooped());
    d->pushButton_VcrLoop->blockSignals(pushButton_VcrLoopBlockSignals);
  }
  else
  {
    qDebug() << "Number of child nodes in the selected hierarchy is 0 in node "<<multidimDataRootNode->GetID();
    d->slider_ParameterValue->setEnabled(false);
  }   

  int selectedBundleIndex=d->activeBrowserNode()->GetSelectedBundleIndex();
  if (selectedBundleIndex>0)
  {
    std::string parameterValue=multidimDataRootNode->GetNthParameterValue(selectedBundleIndex);
    if (!parameterValue.empty())
    {
      d->label_ParameterValue->setText(parameterValue.c_str());
      d->slider_ParameterValue->setValue(selectedBundleIndex);
    }
    else
    {
      qWarning() << "Bundle "<<selectedBundleIndex<<" has no parameter value defined";
      d->label_ParameterValue->setText("");
      d->slider_ParameterValue->setValue(0);
    }  
  }
  else
  {
    d->label_ParameterValue->setText("");
    d->slider_ParameterValue->setValue(0);
  }  
  
  foreach( QObject*w, vcrControls ) { w->setProperty( "enabled", vcrControlsEnabled ); }
  if (vcrControlsEnabled)
  {
    if (d->activeBrowserNode()->GetPlaybackActive() && d->activeBrowserNode()->GetPlaybackRateFps()>0)
    {
      int delayInMsec=1000.0/d->activeBrowserNode()->GetPlaybackRateFps();
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
  */
}
