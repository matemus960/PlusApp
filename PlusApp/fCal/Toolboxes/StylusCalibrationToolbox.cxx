/*=Plus=header=begin======================================================
Program: Plus
Copyright (c) Laboratory for Percutaneous Surgery. All rights reserved.
See License.txt for details.
=========================================================Plus=header=end*/ 

#include "StylusCalibrationToolbox.h"

#include "fCalMainWindow.h"
#include "vtkToolVisualizer.h"
#include "vtkTrackedFrameList.h"
#include "TrackedFrame.h"

#include "vtkPivotCalibrationAlgo.h"
#include "ConfigFileSaverDialog.h"

#include <QFileDialog>
#include <QTimer>

#include "vtkMath.h"

static std::string stylusTipName = "StylusTip";

//-----------------------------------------------------------------------------

StylusCalibrationToolbox::StylusCalibrationToolbox(fCalMainWindow* aParentMainWindow, Qt::WFlags aFlags)
  : AbstractToolbox(aParentMainWindow)
  , QWidget(aParentMainWindow, aFlags)
  , m_NumberOfPoints(200)
  , m_CurrentPointNumber(0)
  , m_StylusToolName("")
  , m_StylusPositionString("")
{
  ui.setupUi(this);

  // Create algorithm class
  m_PivotCalibration = vtkPivotCalibrationAlgo::New();
  if (m_PivotCalibration == NULL) {
    LOG_ERROR("Unable to instantiate pivot calibration algorithm class!");
    return;
  }

  // Feed number of points from controller
  ui.spinBox_NumberOfStylusCalibrationPoints->setValue(m_NumberOfPoints);

  // Connect events
  connect( ui.pushButton_Start, SIGNAL( clicked() ), this, SLOT( Start() ) );
  connect( ui.pushButton_Stop, SIGNAL( clicked() ), this, SLOT( Stop() ) );
  connect( ui.spinBox_NumberOfStylusCalibrationPoints, SIGNAL( valueChanged(int) ), this, SLOT( NumberOfStylusCalibrationPointsChanged(int) ) );
}

//-----------------------------------------------------------------------------

StylusCalibrationToolbox::~StylusCalibrationToolbox()
{
  if (m_PivotCalibration != NULL) {
    m_PivotCalibration->Delete();
    m_PivotCalibration = NULL;
  } 
}

//-----------------------------------------------------------------------------

void StylusCalibrationToolbox::Initialize()
{
  LOG_TRACE("StylusCalibrationToolbox::Initialize");

  if (m_State == ToolboxState_Done)
  {
    SetDisplayAccordingToState();
    return;
  }

  // Clear results poly data
  m_ParentMainWindow->GetToolVisualizer()->GetResultPolyData()->Initialize();

  if ( (m_ParentMainWindow->GetToolVisualizer()->GetDataCollector() != NULL)
    && (m_ParentMainWindow->GetToolVisualizer()->GetDataCollector()->GetConnected()))
  {
    m_ParentMainWindow->GetToolVisualizer()->GetDataCollector()->SetTrackingOnly(true);

    if (ReadConfiguration(vtkPlusConfig::GetInstance()->GetDeviceSetConfigurationData()) != PLUS_SUCCESS)
    {
      LOG_ERROR("Reading stylus calibration configuration failed!");
      return;
    }

    // Set initialized if it was uninitialized
    if (m_State == ToolboxState_Uninitialized)
    {
      SetState(ToolboxState_Idle);
    }
    else
    {
      SetDisplayAccordingToState();
    }
  }
  else
  {
    SetState(ToolboxState_Uninitialized);
  }
}

//-----------------------------------------------------------------------------

PlusStatus StylusCalibrationToolbox::ReadConfiguration(vtkXMLDataElement* aConfig)
{
  LOG_TRACE("StylusCalibrationToolbox::ReadConfiguration");

  if (aConfig == NULL)
  {
    LOG_ERROR("Unable to read configuration"); 
    return PLUS_FAIL; 
  }

  vtkXMLDataElement* fCalElement = aConfig->FindNestedElementWithName("fCal"); 

  if (fCalElement == NULL)
  {
    LOG_ERROR("Unable to find fCal element in XML tree!"); 
    return PLUS_FAIL;     
  }

  // Number of stylus calibraiton points to acquire
  int numberOfStylusCalibrationPointsToAcquire = 0; 
  if ( fCalElement->GetScalarAttribute("NumberOfStylusCalibrationPointsToAcquire", numberOfStylusCalibrationPointsToAcquire ) )
  {
    m_NumberOfPoints = numberOfStylusCalibrationPointsToAcquire;
    ui.spinBox_NumberOfStylusCalibrationPoints->setValue(m_NumberOfPoints);
  }

  // Stylus tool name
  vtkXMLDataElement* trackerToolNames = fCalElement->FindNestedElementWithName("TrackerToolNames"); 

  if (trackerToolNames == NULL)
  {
    LOG_ERROR("Unable to find TrackerToolNames element in XML tree!"); 
    return PLUS_FAIL;     
  }

  const char* stylusToolName = trackerToolNames->GetAttribute("Stylus");
  if (stylusToolName == NULL)
  {
	  LOG_ERROR("Stylus tool name is not specified in the fCal section of the configuration!");
    return PLUS_FAIL;     
  }

  m_StylusToolName = std::string(stylusToolName);

  // Check if a tool with the specified name exists
  if (m_ParentMainWindow->GetToolVisualizer()->GetDataCollector() == NULL || m_ParentMainWindow->GetToolVisualizer()->GetDataCollector()->GetTrackingEnabled() == false)
  {
    LOG_ERROR("Data collector object is invalid or not tracking!");
    return PLUS_FAIL;
  }

  TrackedFrame trackedFrame;
  if (m_ParentMainWindow->GetToolVisualizer()->GetDataCollector()->GetTrackedFrame(&trackedFrame) != PLUS_SUCCESS)
  {
    LOG_ERROR("Unable to get tracked frame from data collector!");
    return PLUS_FAIL;
  }

  // TODO
  LOG_ERROR("TEMPORARY ISSUE: TransformRepository will check the availability of the stylus tool");
  bool stylusFound = false;
  if (!stylusFound)
  {
    LOG_ERROR("No tool found with the specified name '" << m_StylusToolName << "'!");
    return PLUS_FAIL;
  }

  return PLUS_SUCCESS;
}

//-----------------------------------------------------------------------------

void StylusCalibrationToolbox::RefreshContent()
{
  //LOG_TRACE("StylusCalibrationToolbox: Refresh stylus calibration toolbox content"); 

  if (m_State == ToolboxState_Idle)
  {
    ui.label_NumberOfPoints->setText(QString("%1 / %2").arg(0).arg(m_NumberOfPoints));
    ui.label_CurrentPosition->setText(m_StylusPositionString.c_str());
  }
  else if (m_State == ToolboxState_InProgress)
  {
    ui.label_NumberOfPoints->setText(QString("%1 / %2").arg(m_CurrentPointNumber).arg(m_NumberOfPoints));
    ui.label_CurrentPosition->setText(m_StylusPositionString.c_str());
    m_ParentMainWindow->SetStatusBarProgress((int)(100.0 * (m_CurrentPointNumber / (double)m_NumberOfPoints) + 0.5));
  }
  else if (m_State == ToolboxState_Done)
  {
    ui.label_CurrentPosition->setText(m_ParentMainWindow->GetToolVisualizer()->GetToolPositionString(m_StylusToolName.c_str(), true).c_str());
  }
}

//-----------------------------------------------------------------------------

void StylusCalibrationToolbox::SetDisplayAccordingToState()
{
  LOG_TRACE("StylusCalibrationToolbox::SetDisplayAccordingToState"); 

  m_ParentMainWindow->GetToolVisualizer()->EnableImageMode(false);
  m_ParentMainWindow->GetToolVisualizer()->HideAll();

  if (m_State == ToolboxState_Uninitialized)
  {
    ui.label_NumberOfPoints->setText(QString("%1 / %2").arg(0).arg(m_NumberOfPoints));
    ui.label_CalibrationError->setText(tr("N/A"));
    ui.label_CurrentPosition->setText(tr("N/A"));
    ui.label_StylusTipTransform->setText(tr("N/A"));
    ui.label_Instructions->setText(tr(""));

    ui.pushButton_Start->setEnabled(false);
    ui.pushButton_Stop->setEnabled(false);

    m_ParentMainWindow->SetStatusBarText(QString(""));
    m_ParentMainWindow->SetStatusBarProgress(-1);
  }
  else if (m_State == ToolboxState_Idle)
  {
    ui.label_CalibrationError->setText(tr("N/A"));
    ui.label_CurrentPosition->setText(tr("N/A"));
    ui.label_StylusTipTransform->setText(tr("N/A"));
    ui.label_Instructions->setText(tr("Put stylus so that its tip is in steady position, and press Start"));

    ui.pushButton_Start->setEnabled(true);
    ui.pushButton_Stop->setEnabled(false);

    ui.pushButton_Start->setFocus();

    m_ParentMainWindow->SetStatusBarText(QString(""));
    m_ParentMainWindow->SetStatusBarProgress(-1);
  }
  else if (m_State == ToolboxState_InProgress)
  {
    ui.label_NumberOfPoints->setText(QString("%1 / %2").arg(m_CurrentPointNumber).arg(m_NumberOfPoints));
    ui.label_CalibrationError->setText(tr("N/A"));
    ui.label_CurrentPosition->setText(m_StylusPositionString.c_str());
    ui.label_StylusTipTransform->setText(tr("N/A"));
    ui.label_Instructions->setText(tr("Move around stylus with its tip fixed until the required amount of points are aquired"));

    ui.pushButton_Start->setEnabled(false);
    ui.pushButton_Stop->setEnabled(true);

    m_ParentMainWindow->SetStatusBarText(QString(" Recording stylus positions"));
    m_ParentMainWindow->SetStatusBarProgress(0);

    m_ParentMainWindow->GetToolVisualizer()->ShowInput(true);

    ui.pushButton_Stop->setFocus();
  }
  else if (m_State == ToolboxState_Done)
  {
    ui.pushButton_Start->setEnabled(true);
    ui.pushButton_Stop->setEnabled(false);
    ui.label_Instructions->setText(tr("Calibration transform is ready to save"));

    ui.label_NumberOfPoints->setText(QString("%1 / %2").arg(m_CurrentPointNumber).arg(m_NumberOfPoints));
    ui.label_CalibrationError->setText(QString("%1 mm").arg(m_PivotCalibration->GetCalibrationError(), 2));
    ui.label_CurrentPosition->setText(m_StylusPositionString.c_str());
    ui.label_StylusTipTransform->setText(m_PivotCalibration->GetTooltipToToolTranslationString().c_str());

    m_ParentMainWindow->SetStatusBarText(QString(" Stylus calibration done"));
    m_ParentMainWindow->SetStatusBarProgress(-1);

    m_ParentMainWindow->GetToolVisualizer()->ShowInput(true);
    m_ParentMainWindow->GetToolVisualizer()->ShowResult(true);
    m_ParentMainWindow->GetToolVisualizer()->GetCanvasRenderer()->ResetCamera();
    m_ParentMainWindow->GetToolVisualizer()->ShowTool(m_StylusToolName.c_str(), true);

    QApplication::restoreOverrideCursor();
  }
  else if (m_State == ToolboxState_Error)
  {
    ui.pushButton_Start->setEnabled(true);
    ui.pushButton_Stop->setEnabled(false);
    ui.label_Instructions->setText(tr(""));

    ui.label_NumberOfPoints->setText(tr("N/A"));
    ui.label_CalibrationError->setText(tr("N/A"));
    ui.label_CurrentPosition->setText(tr("N/A"));
    ui.label_StylusTipTransform->setText(tr("N/A"));

    m_ParentMainWindow->SetStatusBarText(QString(""));
    m_ParentMainWindow->SetStatusBarProgress(-1);

    QApplication::restoreOverrideCursor();
  }
}

//-----------------------------------------------------------------------------

void StylusCalibrationToolbox::Start()
{
  LOG_TRACE("StylusCalibrationToolbox::StartClicked"); 

  m_ParentMainWindow->SetTabsEnabled(false);
  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  m_CurrentPointNumber = 0;

  // Clear input points and result point
  vtkSmartPointer<vtkPoints> inputPoints = vtkSmartPointer<vtkPoints>::New();
  m_ParentMainWindow->GetToolVisualizer()->GetInputPolyData()->SetPoints(inputPoints);

  vtkSmartPointer<vtkPoints> resultPointsPoint = vtkSmartPointer<vtkPoints>::New();
  m_ParentMainWindow->GetToolVisualizer()->GetResultPolyData()->SetPoints(resultPointsPoint);

  // Initialize calibration
  m_PivotCalibration->Initialize();

  // Initialize stylus tool
  vtkDisplayableTool* stylusDisplayable = NULL;
  if (m_ParentMainWindow->GetToolVisualizer()->GetDisplayableTool(m_StylusToolName.c_str(), stylusDisplayable) != PLUS_SUCCESS)
  {
    LOG_ERROR("Stylus tool not found!");
    return;
  }

  // Set state to in progress
  SetState(ToolboxState_InProgress);

  // Connect acquisition function to timer
  connect( m_ParentMainWindow->GetToolVisualizer()->GetAcquisitionTimer(), SIGNAL( timeout() ), this, SLOT( AddStylusPositionToCalibration() ) );
}

//-----------------------------------------------------------------------------

void StylusCalibrationToolbox::Stop()
{
  LOG_TRACE("StylusCalibrationToolbox::StopClicked"); 

  // Disonnect acquisition function to timer
  disconnect( m_ParentMainWindow->GetToolVisualizer()->GetAcquisitionTimer(), SIGNAL( timeout() ), this, SLOT( AddStylusPositionToCalibration() ) );

  // Calibrate
  PlusStatus success = m_PivotCalibration->DoTooltipCalibration();

  if (success == PLUS_SUCCESS)
  {
    LOG_INFO("Stylus calibration successful");

    // Feed result to tool
    vtkDisplayableTool* stylusDisplayable = NULL;
    if (m_ParentMainWindow->GetToolVisualizer()->GetDisplayableTool(m_StylusToolName.c_str(), stylusDisplayable) != PLUS_SUCCESS)
    {
      LOG_ERROR("Stylus tool not found!");
      SetState(ToolboxState_Error);
      return;
    }

    // Add pivot calibration result to transform repository
    PlusTransformName stylusCalibrationMatrixName(stylusTipName.c_str(), m_StylusToolName.c_str());
    m_ParentMainWindow->GetToolVisualizer()->GetTransformRepository()->SetTransform(stylusCalibrationMatrixName, m_PivotCalibration->GetTooltipToToolTransformMatrix());

    // Set result point
    double stylustipPosition[3];
    m_PivotCalibration->GetTooltipPosition(stylustipPosition);
    vtkPoints* points = m_ParentMainWindow->GetToolVisualizer()->GetResultPolyData()->GetPoints();
    points->InsertPoint(0, stylustipPosition);
    points->Modified();

    // Write result in configuration
    if ( vtkPlusConfig::WriteTransformToCoordinateDefinition(stylusTipName.c_str(), m_StylusToolName.c_str(), m_PivotCalibration->GetTooltipToToolTransformMatrix(), 
      m_PivotCalibration->GetCalibrationError() ) != PLUS_SUCCESS )
    {
      LOG_ERROR("Unable to save stylus calibration result in configuration XML tree!");
      SetState(ToolboxState_Error);
      return;
    }

    SetState(ToolboxState_Done);
  }
  else
  {
    LOG_ERROR("Stylus calibration failed!");

    m_CurrentPointNumber = 0;
    SetState(ToolboxState_Error);
  }

  m_ParentMainWindow->SetTabsEnabled(true);
}

//-----------------------------------------------------------------------------

void StylusCalibrationToolbox::NumberOfStylusCalibrationPointsChanged(int aNumberOfPoints)
{
  LOG_TRACE("StylusCalibrationToolbox::NumberOfStylusCalibrationPointsChanged");

  m_NumberOfPoints = aNumberOfPoints;
}

//-----------------------------------------------------------------------------

void StylusCalibrationToolbox::AddStylusPositionToCalibration()
{
  LOG_TRACE("StylusCalibrationToolbox::AddStylusPositionToCalibration");

  vtkSmartPointer<vtkMatrix4x4> stylusToReferenceMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  if (m_ParentMainWindow->GetToolVisualizer()->AcquireTrackerPositionForToolByName(m_StylusToolName.c_str(), stylusToReferenceMatrix) == TR_OK)
  {
    double stylusPosition[4]={stylusToReferenceMatrix->GetElement(0,3), stylusToReferenceMatrix->GetElement(1,3), stylusToReferenceMatrix->GetElement(2,3), 1.0 };

    // Assemble position string for toolbox
    char stylusPositionChars[32];

    sprintf_s(stylusPositionChars, 32, "%.1lf X %.1lf X %.1lf", stylusPosition[0], stylusPosition[1], stylusPosition[2]);
    m_StylusPositionString = std::string(stylusPositionChars);

    // Add point to the input if fulfills the criteria
    vtkPoints* points = m_ParentMainWindow->GetToolVisualizer()->GetInputPolyData()->GetPoints();

    double distance_lowThreshold_mm = 2.0; // TODO: review these thresholds
    double distance_highThreshold_mm = 1000.0;
    double distance = -1.0;
    if (m_CurrentPointNumber < 1)
    {
      // Always allow
      distance = (distance_lowThreshold_mm + distance_highThreshold_mm) / 2.0;
    }
    else
    {
      double previousPosition[4];
      points->GetPoint(m_CurrentPointNumber-1, previousPosition);
      previousPosition[3] = 1.0;
      // Square distance
      distance = vtkMath::Distance2BetweenPoints(stylusPosition, previousPosition);
    }

    // If current point is close to the previous one, or too far (outlier), we do not insert it
    if (distance < distance_lowThreshold_mm * distance_lowThreshold_mm)
    {
      LOG_DEBUG("Acquired position is too close to the previous - it is skipped");
    }
    else if (distance > distance_highThreshold_mm * distance_highThreshold_mm)
    {
      LOG_DEBUG("Acquired position seems to be an outlier - it is skipped");
    }
    else
    {
      // Add the point into the calibration dataset
      m_PivotCalibration->InsertNextCalibrationPoint(stylusToReferenceMatrix);

      // Add to polydata for rendering
      points->InsertPoint(m_CurrentPointNumber, stylusPosition[0], stylusPosition[1], stylusPosition[2]);
      points->Modified();

      // Set new current point number
      ++m_CurrentPointNumber;

      // Reset the camera once in a while
      if ((m_CurrentPointNumber > 0) && ((m_CurrentPointNumber % 10 == 0) || (m_CurrentPointNumber == 5) || (m_CurrentPointNumber >= m_NumberOfPoints)))
      {
        m_ParentMainWindow->GetToolVisualizer()->GetCanvasRenderer()->ResetCamera();
      }

      // If enough points have been acquired, stop
      if (m_CurrentPointNumber >= m_NumberOfPoints)
      {
        Stop();
      }
    }
  }
}
