/*=========================================================================
 *
 *  Copyright IGT Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#include "ui_InVivoTrackingOffline.h"
#include "InVivoTrackingOffline.h"
#include <QFileDialog>
#include <qmessagebox.h>
#include <qtextstream.h>
#include <qpen.h>
#include <qcustomplot.h>

// Constructor
InVivoTrackingOffline::InVivoTrackingOffline()
{
	this->ui = new Ui_InVivoTrackingOffline;
	this->ui->setupUi(this);
	this->move(20, 20);

	qRegisterMetaType<MatrixType>("VectorType");
	qRegisterMetaType<MatrixType>("MatrixType");

	m_ProjViewMin = 0;
	m_ProjViewMax = 6;
	m_DiffViewMin = -1;
	m_DiffViewMax = 1;

	m_DefaultPath = QDir::homePath();

	m_PlotLength = 200;

	// CUDA
#ifdef IGT_USE_CUDA
	ui->gpuComboBox->addItem("CUDA");
	ui->gpuComboBox->setCurrentText("CUDA");
#endif

	// Target tracker
	m_TargetTracker = new TargetTracker;

	/** Graphs. ================ */
	// 0 - 2: tracking results
	// 3 - 5: prior
	// 6 - 8: ground truth (if provided)
	QPen penTracked;
	QPen penPrior;
	QPen penGT;
	penTracked.setColor(QColor(0, 0, 255));
	penTracked.setStyle(Qt::SolidLine);
	penTracked.setWidthF(2);
	penPrior.setColor(QColor(255, 0, 0));
	penPrior.setStyle(Qt::SolidLine);
	penPrior.setWidthF(2);
	penGT.setColor(QColor(0, 255, 0));
	penGT.setStyle(Qt::SolidLine);
	penGT.setWidthF(2);

#define SETUP_PLOT(plot) \
	plot->xAxis->setLabel("Projection number"); \
	plot->xAxis->setLabelFont(QFont("Courier")); \
	plot->yAxis->setLabelFont(QFont("Courier", 12)); \
	plot->xAxis->setTickLabelFont(QFont("Courier")); \
	plot->yAxis->setTickLabelFont(QFont("Courier")); \
	for (unsigned int k = 0; k != 3; ++k) \
		plot->addGraph(); \
	plot->graph(0)->setPen(penTracked); \
	plot->graph(0)->setName("Tracking"); \
	plot->graph(1)->setPen(penPrior); \
	plot->graph(1)->setName("Prior"); \
	plot->graph(2)->setPen(penGT); \
	plot->graph(2)->setName("Ground truth")

	SETUP_PLOT(ui->lrPlot);
	SETUP_PLOT(ui->siPlot);
	SETUP_PLOT(ui->apPlot);
	SETUP_PLOT(ui->inPlanePlot);
	SETUP_PLOT(ui->inDepthPlot);
	SETUP_PLOT(ui->legendPlot);

	ui->lrPlot->yAxis->setLabel("LR (mm)");
	ui->siPlot->yAxis->setLabel("SI (mm)");
	ui->apPlot->yAxis->setLabel("AP (mm)");
	ui->inPlanePlot->yAxis->setLabel("Lateral (mm)");
	ui->inDepthPlot->yAxis->setLabel("In-depth (mm)");

	ui->legendPlot->xAxis->setVisible(false);
	ui->legendPlot->yAxis->setVisible(false);
	ui->legendPlot->legend->setFont(QFont("Courier"));
	ui->legendPlot->legend->setVisible(true);
	ui->legendPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignCenter | Qt::AlignHCenter);
	/** Graphs. ================ */

	// Tracking mask display
	m_TrackingMaskColorTable.push_back(qRgba(0, 0, 0, 0));
	for (unsigned int k = 1; k != 256; ++k)
		m_TrackingMaskColorTable.push_back(qRgba(0, 0, 255, 150));

	// Connect tracking thread with main window
	m_TargetTracker->moveToThread(&trackingThread);
	connect(&trackingThread, &QThread::finished, m_TargetTracker, &QObject::deleteLater);
	connect(this, &InVivoTrackingOffline::startTrackingSignal, m_TargetTracker, &TargetTracker::doTracking);
	connect(m_TargetTracker, &TargetTracker::trackingDoneSignal, this, &InVivoTrackingOffline::onTrackingDone);
	connect(m_TargetTracker, &TargetTracker::trackingErrorSignal, this, &InVivoTrackingOffline::onTrackingError);
	connect(m_TargetTracker, &TargetTracker::updateProgramStatusSignal, this, &InVivoTrackingOffline::updateProgramStatus);
	connect(m_TargetTracker, &TargetTracker::updateFrameNumberSignal, this, &InVivoTrackingOffline::updateFrameNumber);
	connect(m_TargetTracker, &TargetTracker::updateProjAngleSignal, this, &InVivoTrackingOffline::updateProjAngle);
	connect(m_TargetTracker, &TargetTracker::updateBinNumberSignal, this, &InVivoTrackingOffline::updateBinNumber);
	connect(m_TargetTracker, &TargetTracker::updateReadingTimeSignal, this, &InVivoTrackingOffline::updateReadingTime);
	connect(m_TargetTracker, &TargetTracker::updateTrackingTimeSignal, this, &InVivoTrackingOffline::updateTrackingTime);
	connect(m_TargetTracker, &TargetTracker::updateTotalTimeSignal, this, &InVivoTrackingOffline::updateTotalTime);
	connect(m_TargetTracker, &TargetTracker::updateStateCovarianceSignal, this, &InVivoTrackingOffline::updateStateCovariance);
	connect(m_TargetTracker, &TargetTracker::updateMatchingMetricSignal, this, &InVivoTrackingOffline::updateMatchingMetric);
	connect(m_TargetTracker, &TargetTracker::updateBGMatchScoreSignal, this, &InVivoTrackingOffline::updateBGMatchScore);
	connect(m_TargetTracker, &TargetTracker::resultReadySignal, this, &InVivoTrackingOffline::processResults);
	connect(m_TargetTracker, &TargetTracker::projViewReadySignal, this, &InVivoTrackingOffline::setProjViewPixmap);
	connect(m_TargetTracker, &TargetTracker::writeDataPropertiesSignal, this, &InVivoTrackingOffline::writeDataProperties);
	trackingThread.start();
};

InVivoTrackingOffline::~InVivoTrackingOffline()
{
  // The smart pointers should clean up for up
	trackingThread.quit();
	trackingThread.wait();
}

// Check required inputs and enable start tracking button
void InVivoTrackingOffline::checkStartRequirement()
{
	if ( !ui->projDirOKLabel->text().contains("Not set")
		&& !ui->targetDirOKLabel->text().contains("Not set")
		&& ui->geometryFileOKLabel->text().contains("OK")
		&& ui->sortFileOKLabel->text().contains("OK")
		&& (ui->logDirOKLabel->text().contains("OK") || !ui->writeResultsCheckBox->isChecked())
		)
		ui->startPushButton->setEnabled(true);
	else
		ui->startPushButton->setEnabled(false);
}

// Enable/Disable input fields
void InVivoTrackingOffline::enableInputFields()
{
	ui->projDirTextEdit->setEnabled(true);
	ui->projDirPushButton->setEnabled(true);
	ui->targetDirTextEdit->setEnabled(true);
	ui->targetDirPushButton->setEnabled(true);
	ui->geometryFileTextEdit->setEnabled(true);
	ui->geometryFilePushButton->setEnabled(true);
	ui->sortFileTextEdit->setEnabled(true);
	ui->sortFilePushButton->setEnabled(true);
	ui->writeResultsCheckBox->setEnabled(true);
	if (ui->writeResultsCheckBox->isChecked())
	{
		ui->logDirTextEdit->setEnabled(true);
		ui->logDirPushButton->setEnabled(true);
	}
	else
	{
		ui->logDirTextEdit->setEnabled(false);
		ui->logDirPushButton->setEnabled(false);
	}
	ui->surrogateFileTextEdit->setEnabled(true);
	ui->surrogateFilePushButton->setEnabled(true);
	ui->metricComboBox->setEnabled(true);
	ui->gpuComboBox->setEnabled(true);
	ui->bgDirTextEdit->setEnabled(true);
	ui->bgDirPushButton->setEnabled(true);
	ui->gtFilePushButton->setEnabled(true);
	ui->gtFileTextEdit->setEnabled(true);
	ui->startSpineBox->setEnabled(true);
	ui->stepSpineBox->setEnabled(true);
	ui->endSpineBox->setEnabled(true);
	ui->matchThresholdDoubleSpineBox->setEnabled(true);
	ui->gtCheckCheckBox->setEnabled(true);
}

void InVivoTrackingOffline::disableInputFields()
{
	ui->projDirTextEdit->setEnabled(false);
	ui->projDirPushButton->setEnabled(false);
	ui->targetDirTextEdit->setEnabled(false);
	ui->targetDirPushButton->setEnabled(false);
	ui->geometryFileTextEdit->setEnabled(false);
	ui->geometryFilePushButton->setEnabled(false);
	ui->sortFileTextEdit->setEnabled(false);
	ui->sortFilePushButton->setEnabled(false);
	ui->logDirTextEdit->setEnabled(false);
	ui->logDirPushButton->setEnabled(false);
	ui->surrogateFileTextEdit->setEnabled(false);
	ui->surrogateFilePushButton->setEnabled(false);
	ui->writeResultsCheckBox->setEnabled(false);
	ui->metricComboBox->setEnabled(false);
	ui->gpuComboBox->setEnabled(false);
	ui->bgDirTextEdit->setEnabled(false);
	ui->bgDirPushButton->setEnabled(false);
	ui->gtFilePushButton->setEnabled(false);
	ui->gtFileTextEdit->setEnabled(false);
	ui->startSpineBox->setEnabled(false);
	ui->stepSpineBox->setEnabled(false);
	ui->endSpineBox->setEnabled(false);
	ui->matchThresholdDoubleSpineBox->setEnabled(false);
	ui->gtCheckCheckBox->setEnabled(false);
}

// Projection directory

void InVivoTrackingOffline::on_projDirPushButton_released()
{
	QString projDir_QStr = QFileDialog::getExistingDirectory(this,
		tr("Select the projection directory"),
		m_DefaultPath
		);
	
	if (!projDir_QStr.isEmpty())
		ui->projDirTextEdit->setText(projDir_QStr);
}

void InVivoTrackingOffline::on_projDirTextEdit_textChanged()
{
	if (!ui->projDirTextEdit->toPlainText().isEmpty() &&
		QDir(ui->projDirTextEdit->toPlainText()).exists())
	{
		QStringList parts1 = ui->projDirTextEdit->toPlainText().split("/");
		QStringList parts2 = ui->projDirTextEdit->toPlainText().split("\\");
		QStringList parts = (parts1.size() > parts2.size()) ? parts1 : parts2;
		m_DefaultPath = "";
		for (unsigned int k = 0; k != parts.size() - 1; ++k)
			m_DefaultPath += parts[k] + QDir::separator();

		m_TargetTracker->GetProjFileNames()->SetDirectory(ui->projDirTextEdit->toPlainText().toStdString());
		m_TargetTracker->SetNProj(m_TargetTracker->GetProjFileNames()->GetFileNames().size());
		
		if (m_TargetTracker->GetNProj() == 0)
		{
			ui->projDirOKLabel->setText(QString("Not set"));
			ui->projDirOKLabel->setStyleSheet("QLabel{ color: black; }");
		}
		else
		{
			ui->projDirOKLabel->setText(QString::number(m_TargetTracker->GetNProj()) + QString(" projections"));
			ui->projDirOKLabel->setStyleSheet("QLabel{ color: green; }");
			ui->endSpineBox->setMaximum(m_TargetTracker->GetNProj());
			ui->endSpineBox->setValue(m_TargetTracker->GetNProj());
		}
	}
	else
	{
		m_TargetTracker->SetNProj(0);
		ui->projDirOKLabel->setText("Not set");
		ui->projDirOKLabel->setStyleSheet("QLabel{ color: black; }");
	}

	this->checkStartRequirement();
}

// Target model directory

void InVivoTrackingOffline::on_targetDirPushButton_released()
{
	QString targetDir_QStr = QFileDialog::getExistingDirectory(this,
		tr("Select the target model directory"),
		m_DefaultPath
		);

	if (!targetDir_QStr.isEmpty())
		ui->targetDirTextEdit->setText(targetDir_QStr);
}

void InVivoTrackingOffline::on_targetDirTextEdit_textChanged()
{
	if (!ui->targetDirTextEdit->toPlainText().isEmpty() &&
		QDir(ui->targetDirTextEdit->toPlainText()).exists())
	{
		m_TargetTracker->GetTargetFileNames()->SetDirectory(ui->targetDirTextEdit->toPlainText().toStdString());
		m_TargetTracker->SetNBin(m_TargetTracker->GetTargetFileNames()->GetFileNames().size());

		if (m_TargetTracker->GetNBin() == 0)
		{
			ui->targetDirOKLabel->setText(QString("Not set"));
			ui->targetDirOKLabel->setStyleSheet("QLabel{ color: black; }");
		}
		else
		{
			ui->targetDirOKLabel->setText(QString::number(m_TargetTracker->GetNBin()) + QString(" models"));
			ui->targetDirOKLabel->setStyleSheet("QLabel{ color: green; }");
		}
	}
	else
	{
		m_TargetTracker->SetNBin(0);
		ui->targetDirOKLabel->setText("Not set");
		ui->targetDirOKLabel->setStyleSheet("QLabel{ color: black; }");
	}

	this->on_bgDirTextEdit_textChanged();
	this->checkStartRequirement();
}

// RTK Geometry file

void InVivoTrackingOffline::on_geometryFilePushButton_released()
{
	QString geometryFile_QStr = QFileDialog::getOpenFileName(this,
		tr("Select the RTK geometry file"),
		m_DefaultPath,
		tr("RTK Geometry XML (*.xml)"));
	
	if (!geometryFile_QStr.isEmpty())
		ui->geometryFileTextEdit->setText(geometryFile_QStr);
}

void InVivoTrackingOffline::on_geometryFileTextEdit_textChanged()
{
	if (QFile(ui->geometryFileTextEdit->toPlainText()).exists()
		&& QFileInfo(ui->geometryFileTextEdit->toPlainText()).suffix().compare("xml",Qt::CaseInsensitive) == 0)
	{
		m_TargetTracker->SetGeometryFileName(ui->geometryFileTextEdit->toPlainText().toStdString());
		ui->geometryFileOKLabel->setText(QString("OK"));
		ui->geometryFileOKLabel->setStyleSheet("QLabel{ color: green; }");
	}
	else
	{
		ui->geometryFileOKLabel->setText("Not set");
		ui->geometryFileOKLabel->setStyleSheet("QLabel{ color: black; }");
	}

	this->checkStartRequirement();
}

// Sorting file

void InVivoTrackingOffline::on_sortFilePushButton_released()
{
	QString sortFile_QStr = QFileDialog::getOpenFileName(this,
		tr("Select the projection sorting information file"),
		m_DefaultPath,
		tr("Sorting information file (*.csv)"));

	if (!sortFile_QStr.isEmpty())
		ui->sortFileTextEdit->setText(sortFile_QStr);
}

void InVivoTrackingOffline::on_sortFileTextEdit_textChanged()
{
	if (QFile(ui->sortFileTextEdit->toPlainText()).exists()
		&& QFileInfo(ui->sortFileTextEdit->toPlainText()).suffix().compare("csv", Qt::CaseInsensitive) == 0)
	{
		m_TargetTracker->SetSortFileName(ui->sortFileTextEdit->toPlainText().toStdString());
		ui->sortFileOKLabel->setText(QString("OK"));
		ui->sortFileOKLabel->setStyleSheet("QLabel{ color: green; }");
	}
	else
	{
		ui->sortFileOKLabel->setText("Not set");
		ui->sortFileOKLabel->setStyleSheet("QLabel{ color: black; }");
	}

	this->checkStartRequirement();
}

// Output log directory

void InVivoTrackingOffline::on_logDirPushButton_released()
{ 
	QString logDir_QStr = QFileDialog::getExistingDirectory(this,
		tr("Select where the output log files will be stored"),
		m_DefaultPath
		);
	
	if (!logDir_QStr.isEmpty())
		ui->logDirTextEdit->setText(logDir_QStr);
}

void InVivoTrackingOffline::on_logDirTextEdit_textChanged()
{
	if (!ui->logDirTextEdit->toPlainText().isEmpty() &&
		QDir(ui->logDirTextEdit->toPlainText()).exists())
	{
		m_TargetTracker->SetOutputDir(ui->logDirTextEdit->toPlainText().toStdString());
		ui->logDirOKLabel->setText(QString("OK"));
		ui->logDirOKLabel->setStyleSheet("QLabel{ color: green; }");
	}
	else
	{
		ui->logDirOKLabel->setText("Not set");
		ui->logDirOKLabel->setStyleSheet("QLabel{ color: black; }");
	}

	this->checkStartRequirement();
}

void InVivoTrackingOffline::on_writeResultsCheckBox_stateChanged(int state)
{
	if (state == 0)
	{
		ui->logDirPushButton->setEnabled(false);
		ui->logDirTextEdit->setEnabled(false);
	}
	else
	{
		ui->logDirPushButton->setEnabled(true);
		ui->logDirTextEdit->setEnabled(true);
	}
	this->checkStartRequirement();
}

// Background model directory

void InVivoTrackingOffline::on_bgDirPushButton_released()
{
	QString bgDir_QStr = QFileDialog::getExistingDirectory(this,
		tr("Select the background model directory"),
		m_DefaultPath
		);

	if (!bgDir_QStr.isEmpty())
		ui->bgDirTextEdit->setText(bgDir_QStr);
}

void InVivoTrackingOffline::on_bgDirTextEdit_textChanged()
{
	if (!ui->bgDirTextEdit->toPlainText().isEmpty() && 
		QDir(ui->bgDirTextEdit->toPlainText()).exists())
	{
		m_TargetTracker->GetBGFileNames()->SetDirectory(ui->bgDirTextEdit->toPlainText().toStdString());

		if (m_TargetTracker->GetBGFileNames()->GetFileNames().size() 
			!= m_TargetTracker->GetNBin())
		{
			m_TargetTracker->SetUseBGSubtraction(false);
			ui->bgDirOKLabel->setText(QString("Not set"));
			ui->bgDirOKLabel->setStyleSheet("QLabel{ color: black; }");
		}
		else
		{
			m_TargetTracker->SetUseBGSubtraction(true);
			ui->bgDirOKLabel->setText(QString("OK"));
			ui->bgDirOKLabel->setStyleSheet("QLabel{ color: green; }");
		}
	}
	else
	{
		m_TargetTracker->SetUseBGSubtraction(false);
		ui->bgDirOKLabel->setText("Not set");
		ui->bgDirOKLabel->setStyleSheet("QLabel{ color: black; }");
	}
}

// Ground truth file

void InVivoTrackingOffline::on_gtFilePushButton_released()
{
	QString gtFile_QStr = QFileDialog::getOpenFileName(this,
		tr("Select the ground truth trajectory file"),
		m_DefaultPath,
		tr("Trajectory file (*.csv)"));

	if (!gtFile_QStr.isEmpty())
		ui->gtFileTextEdit->setText(gtFile_QStr);
}

void InVivoTrackingOffline::on_gtFileTextEdit_textChanged()
{
	m_GTFrameNoQVector.clear();
	m_GTLRQVector.clear();
	m_GTSIQVector.clear();
	m_GTAPQVector.clear();
	m_GTLateralQVector.clear();
	m_GTInDepthQVector.clear();

	if (QFile(ui->gtFileTextEdit->toPlainText()).exists()
		&& QFileInfo(ui->gtFileTextEdit->toPlainText()).suffix().compare("csv", Qt::CaseInsensitive) == 0)
	{
		// Read and plot ground truth LR, SI, AP
		this->getGroundTruthLRSIAP();

		ui->gtFileOKLabel->setText(QString("OK"));
		ui->gtFileOKLabel->setStyleSheet("QLabel{ color: green; }");
	}
	else
	{
		ui->gtFileOKLabel->setText("Not set");
		ui->gtFileOKLabel->setStyleSheet("QLabel{ color: black; }");
	}

	this->checkStartRequirement();
}

// Surrogate signal file
void InVivoTrackingOffline::on_surrogateFilePushButton_released()
{
	QString surrogateFile_QStr = QFileDialog::getOpenFileName(this,
		tr("Select surrogate signal info file"),
		m_DefaultPath,
		tr("Surrogate signal file (*.csv)"));

	if (!surrogateFile_QStr.isEmpty())
		ui->surrogateFileTextEdit->setText(surrogateFile_QStr);
}

void InVivoTrackingOffline::on_surrogateFileTextEdit_textChanged()
{
	m_TargetTracker->GetSurrogateSignal().clear();

	if (QFile(ui->surrogateFileTextEdit->toPlainText()).exists()
		&& QFileInfo(ui->surrogateFileTextEdit->toPlainText()).suffix().compare("csv", Qt::CaseInsensitive) == 0)
	{
		ui->surrogateFileOKLabel->setText(QString("OK"));
		ui->surrogateFileOKLabel->setStyleSheet("QLabel{ color: green; }");
	}
	else
	{
		ui->surrogateFileOKLabel->setText("Not set");
		ui->surrogateFileOKLabel->setStyleSheet("QLabel{ color: black; }");
	}

	this->checkStartRequirement();
}

// Start tracking

void InVivoTrackingOffline::on_startPushButton_released()
{
	if (ui->gtCheckCheckBox->isChecked())
		m_TargetTracker->SetGTCheck(true);
	else
		m_TargetTracker->SetGTCheck(false);

	m_TargetTracker->SetStopNow(false);
	ui->startPushButton->setEnabled(false);
	this->disableInputFields();

	// Read surrogate file
	if (ui->surrogateFileOKLabel->text().contains("OK"))
		this->readSurrogateFile();

	// Set GPU option
	m_TargetTracker->SetGPUOption(ui->gpuComboBox->currentText().toStdString());

	// Set template matching metric option
	if (ui->metricComboBox->currentText().compare("MS") == 0)
		m_TargetTracker->SetTemplateMatchingMetricOption(0);
	else if (ui->metricComboBox->currentText().compare("MMI") == 0)
		m_TargetTracker->SetTemplateMatchingMetricOption(1);
	else if (ui->metricComboBox->currentText().compare("NC") == 0)
		m_TargetTracker->SetTemplateMatchingMetricOption(2);

	// Set matching threshold
	m_TargetTracker->SetMatchingThreshold(ui->matchThresholdDoubleSpineBox->value());

	// Set frames to track
	m_TargetTracker->SetFirstFrame(ui->startSpineBox->value() - 1);
	m_TargetTracker->SetFrameStep(ui->stepSpineBox->value());
	m_TargetTracker->SetEndFrame(ui->endSpineBox->value() - 1);

	// Read geometry here to verify
	this->readGeometryFile();

	// Calculate ground truth lateral and in-depth
	this->getGroundTruthLateralInDepth();

	// Clear results
	this->clearResults();

	// First, save current configuration
	if (ui->writeResultsCheckBox->isChecked())
		this->logFilesBeforeStart();

	emit startTrackingSignal();
}

void InVivoTrackingOffline::on_stopPushButton_released()
{
	this->m_TargetTracker->SetStopNow(true);
}

// When tracking finishes

void InVivoTrackingOffline::onTrackingDone(QString message)
{
	QMessageBox::information(this, "Tracking completes", message);
	this->enableInputFields();
	this->checkStartRequirement();
	if (ui->writeResultsCheckBox->isChecked())
		m_TrackingResultsFile->close();
	ui->programStatusLabel->setText("Idle");
	ui->programStatusLabel->setStyleSheet("QLabel{ color: blue; }");
}

// When tracking finishes or fails
void InVivoTrackingOffline::onTrackingError(QString message)
{
	QMessageBox::critical(this, "Error", message);
	this->enableInputFields();
	this->checkStartRequirement();
	if (ui->writeResultsCheckBox->isChecked())
		m_TrackingResultsFile->close();
	ui->programStatusLabel->setText("Idle");
	ui->programStatusLabel->setStyleSheet("QLabel{ color: blue; }");
}

// Update program status
void InVivoTrackingOffline::updateProgramStatus(QString message, QString style)
{
	ui->programStatusLabel->setText(message);
	ui->programStatusLabel->setStyleSheet(style);
}

// Update frame number
void InVivoTrackingOffline::updateFrameNumber(unsigned int k)
{
	ui->currentFrameLabel->setText(QString::number(k));
}

// Update angle
void InVivoTrackingOffline::updateProjAngle(double ang)
{
	ui->currentAngleLabel->setText(QString::number(ang) + QString(" deg"));
}

// Update bin number
void InVivoTrackingOffline::updateBinNumber(unsigned int k)
{
	ui->currentBinLabel->setText(QString::number(k));
}

// Update reading time
void InVivoTrackingOffline::updateReadingTime(double t)
{
	ui->readTimeLabel->setText(QString::number(t*1000) + QString(" ms"));
}

void InVivoTrackingOffline::updateTrackingTime(double t)
{
	ui->trackTimeLabel->setText(QString::number(t*1000) + QString(" ms"));
}

void InVivoTrackingOffline::updateTotalTime(double t)
{
	ui->totalTimeLabel->setText(QString::number(t * 1000) + QString(" ms"));
}

void InVivoTrackingOffline::updateStateCovariance(MatrixType p)
{
	m_StateCovarianceMatrix = p;
	ui->stateCovarianceLabel->setText("<p>" +
		QString("%1").arg(QString::number(p(0, 0), 'f', 2), 8, ' ') + "," + 
		QString("%1").arg(QString::number(p(0, 1), 'f', 2), 8, ' ') + "," +
		QString("%1").arg(QString::number(p(0, 2), 'f', 2), 8, ' ') + "<br/>" +
		QString("%1").arg(QString::number(p(1, 0), 'f', 2), 8, ' ') + "," +
		QString("%1").arg(QString::number(p(1, 1), 'f', 2), 8, ' ') + "," +
		QString("%1").arg(QString::number(p(1, 2), 'f', 2), 8, ' ') + "<br/>" +
		QString("%1").arg(QString::number(p(2, 0), 'f', 2), 8, ' ') + "," +
		QString("%1").arg(QString::number(p(2, 1), 'f', 2), 8, ' ') + "," +
		QString("%1").arg(QString::number(p(2, 2), 'f', 2), 8, ' ') + "</p>");
}

void InVivoTrackingOffline::updateMatchingMetric(double value)
{
	ui->matchingMetricLabel->setText(QString::number(value, 'f', 2));
}

void InVivoTrackingOffline::updateBGMatchScore(double value)
{
	ui->bgMatchScoreLabel->setText(QString::number(value, 'f', 2));
}

// Load configuration
void InVivoTrackingOffline::on_actionLoad_Configuration_triggered()
{
	QString loadFile_QStr = QFileDialog::getOpenFileName(this,
		tr("Select InVivoTrackingOffline configuration file"),
		m_DefaultPath,
		tr("InVivoTrackingOffline configuration file (*.txt)"));

	if (loadFile_QStr.isEmpty())
		return;

	QFile loadFile(loadFile_QStr);
	if (!loadFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QMessageBox::critical(this, "ERROR", "Cannot load parameter file.");
		return;
	}

	QTextStream loadStream(&loadFile);
	QString line = loadStream.readLine();
	while (!line.isNull())
	{
		QString result;
		QStringList parts;
		if (line.contains("Projection Directory:", Qt::CaseSensitive))
		{
			parts = line.split("Projection Directory:");
			result = parts[1].trimmed();
			ui->projDirTextEdit->setText(result);
		}
		else if (line.contains("Target Model Directory:", Qt::CaseSensitive))
		{
			parts = line.split("Target Model Directory:");
			result = parts[1].trimmed();
			ui->targetDirTextEdit->setText(result);
		}
		else if (line.contains("RTK Geometry File:", Qt::CaseSensitive))
		{
			parts = line.split("RTK Geometry File:");
			result = parts[1].trimmed();
			ui->geometryFileTextEdit->setText(result);
		}
		else if (line.contains("Sorting File:", Qt::CaseSensitive))
		{
			parts = line.split("Sorting File:");
			result = parts[1].trimmed();
			ui->sortFileTextEdit->setText(result);
		}
		else if (line.contains("Output Directory:", Qt::CaseSensitive))
		{
			parts = line.split("Output Directory:");
			result = parts[1].trimmed();
			ui->logDirTextEdit->setText(result);
		}
		else if (line.contains("Results Logging:", Qt::CaseSensitive))
		{
			parts = line.split("Results Logging:");
			result = parts[1].trimmed();
			ui->writeResultsCheckBox->setChecked(result.toInt()>0);
		}
		else if (line.contains("Template Matching Metric:", Qt::CaseSensitive))
		{
			parts = line.split("Template Matching Metric:");
			result = parts[1].trimmed();
			ui->metricComboBox->setCurrentText(result);
		}
		else if (line.contains("GPU Acceleration:", Qt::CaseSensitive))
		{
			parts = line.split("GPU Acceleration:");
			result = parts[1].trimmed();
			ui->gpuComboBox->setCurrentText("None");
#ifdef IGT_USE_CUDA
			if (result.compare("CUDA", Qt::CaseInsensitive) == 0)
				ui->gpuComboBox->setCurrentText("CUDA");
#endif
		}
		else if (line.contains("Background Model Directory:", Qt::CaseSensitive))
		{
			parts = line.split("Background Model Directory:");
			result = parts[1].trimmed();
			ui->bgDirTextEdit->setText(result);
		}
		else if (line.contains("Ground Truth File:", Qt::CaseSensitive))
		{
			parts = line.split("Ground Truth File:");
			result = parts[1].trimmed();
			ui->gtFileTextEdit->setText(result);
		}
		else if (line.contains("Surrogate Signal File:", Qt::CaseSensitive))
		{
			parts = line.split("Surrogate Signal File:");
			result = parts[1].trimmed();
			ui->surrogateFileTextEdit->setText(result);
		}
		else if (line.contains("Start Frame:", Qt::CaseSensitive))
		{
			parts = line.split("Start Frame:");
			result = parts[1].trimmed();
			ui->startSpineBox->setValue(result.toUInt());
		}
		else if (line.contains("Frame Increment:", Qt::CaseSensitive))
		{
			parts = line.split("Frame Increment:");
			result = parts[1].trimmed();
			ui->stepSpineBox->setValue(result.toUInt());
		}
		else if (line.contains("End Frame:", Qt::CaseSensitive))
		{
			parts = line.split("End Frame:");
			result = parts[1].trimmed();
			ui->endSpineBox->setValue(result.toUInt());
		}
		else if (line.contains("Matching Threshold:", Qt::CaseSensitive))
		{
			parts = line.split("Matching Threshold:");
			result = parts[1].trimmed();
			ui->matchThresholdDoubleSpineBox->setValue(result.toDouble());
		}

		line = loadStream.readLine();
	}
	loadFile.close();
}

// When save configuration action is clicked
void InVivoTrackingOffline::on_actionSave_Configuration_triggered()
{
	QString saveFile_QStr = QFileDialog::getSaveFileName(this,
		tr("Save InVivoTrackingOffline configuration to a text file"),
		m_DefaultPath,
		tr("InVivoTrackingOffline configuration file (*.txt)"));

	if (saveFile_QStr.isEmpty())
		return;

	this->saveConfiguration(saveFile_QStr);
}

// Save configuration
void InVivoTrackingOffline::saveConfiguration(QString saveFile_QStr)
{
	QFile saveFile(saveFile_QStr);
	if (!saveFile.open(QIODevice::ReadWrite | QIODevice::Text))
	{
		QMessageBox::critical(this, "ERROR", "Cannot save parameter file.");
		return;
	}

	QTextStream saveStream(&saveFile);

	saveStream << "# REQUIRED INPUTS" << endl;
	if (!ui->projDirTextEdit->toPlainText().isEmpty())
		saveStream << "Projection Directory:			" << ui->projDirTextEdit->toPlainText() << endl;
	if (!ui->targetDirTextEdit->toPlainText().isEmpty())
		saveStream << "Target Model Directory:			" << ui->targetDirTextEdit->toPlainText() << endl;
	if (!ui->geometryFileTextEdit->toPlainText().isEmpty())
		saveStream << "RTK Geometry File:		 		" << ui->geometryFileTextEdit->toPlainText() << endl;
	if (!ui->sortFileTextEdit->toPlainText().isEmpty())
		saveStream << "Sorting File:					" << ui->sortFileTextEdit->toPlainText() << endl;
	if (!ui->logDirTextEdit->toPlainText().isEmpty())
		saveStream << "Output Directory:				" << ui->logDirTextEdit->toPlainText() << endl;
	saveStream << "Results Logging:                " << ui->writeResultsCheckBox->isChecked() << endl;
	if (!ui->targetDirTextEdit->toPlainText().isEmpty())
		saveStream << "Target Model Directory:			" << ui->targetDirTextEdit->toPlainText() << endl;

	saveStream << endl;

	saveStream << "# OPTIONAL INPUTS" << endl;
	if (!ui->bgDirTextEdit->toPlainText().isEmpty())
		saveStream << "Background Model Directory:		" << ui->bgDirTextEdit->toPlainText() << endl;
	if (!ui->gtFileTextEdit->toPlainText().isEmpty())
		saveStream << "Ground Truth File:		        " << ui->gtFileTextEdit->toPlainText() << endl;
	if (!ui->surrogateFileTextEdit->toPlainText().isEmpty())
		saveStream << "Surrogate Signal File:		    " << ui->surrogateFileTextEdit->toPlainText() << endl;
	saveStream << "Template Matching Metric:       " << ui->metricComboBox->currentText() << endl;
	saveStream << "Matching Threshold:			    " << ui->matchThresholdDoubleSpineBox->value() << endl;
	saveStream << "GPU Acceleration:				" << ui->gpuComboBox->currentText() << endl;
	saveStream << "Start Frame:				    " << ui->startSpineBox->value() << endl;
	saveStream << "Frame Increment:				" << ui->stepSpineBox->value() << endl;
	saveStream << "End Frame:				        " << ui->endSpineBox->value() << endl;

	saveFile.close();
}

// Process results
void InVivoTrackingOffline::processResults(unsigned int k_frame, unsigned int currentBin, double angle)
{
	this->plotResults(k_frame, currentBin, angle);

	if (ui->writeResultsCheckBox->isChecked())
		this->writeResults(k_frame, currentBin, angle);
}

void InVivoTrackingOffline::plotResults(unsigned int k_frame, unsigned int currentBin, double angle)
{
	unsigned int k_count = (k_frame - ui->startSpineBox->value() + 1) / ui->stepSpineBox->value();

	m_PlotFrameNoQVector.push_back(k_frame + 1);
	m_MatchingMetricValues.push_back(m_TargetTracker->GetMatchingMetricValues()[k_count]);
	m_BGMatchScoreValues.push_back(m_TargetTracker->GetBGMatchScoreValues()[k_count]);
	m_TrackingLRQVector.push_back(m_TargetTracker->GetTrackingTrj()[k_count](0));
	m_TrackingSIQVector.push_back(-(m_TargetTracker->GetTrackingTrj()[k_count](1)));
	m_TrackingAPQVector.push_back(m_TargetTracker->GetTrackingTrj()[k_count](2));
	m_PriorLRQVector.push_back(m_TargetTracker->GetPriorTrj()[k_count](0));
	m_PriorSIQVector.push_back(-(m_TargetTracker->GetPriorTrj()[k_count](1)));
	m_PriorAPQVector.push_back(m_TargetTracker->GetPriorTrj()[k_count](2));

	VectorType trackingProjPos = m_GeometryReader->GetOutputObject()->
		CalculateProjSpaceCoordinate(m_TargetTracker->GetTrackingTrj()[k_count], k_frame);
	m_TrackingLateralQVector.push_back(trackingProjPos[0]);
	m_TrackingInDepthQVector.push_back(trackingProjPos[2]);

	VectorType priorProjPos = m_GeometryReader->GetOutputObject()->
		CalculateProjSpaceCoordinate(m_TargetTracker->GetPriorTrj()[k_count], k_frame);
	m_PriorLateralQVector.push_back(priorProjPos[0]);
	m_PriorInDepthQVector.push_back(priorProjPos[2]);

	unsigned int start = 0;
	unsigned int end = m_PlotFrameNoQVector.size() - 1;
	unsigned int normalizedPlotLength = m_PlotLength / ui->stepSpineBox->value();
	if (m_PlotFrameNoQVector.size() > normalizedPlotLength)
		start = end - normalizedPlotLength + 1;

	QVector<double> plotFrameNoQVector, trackingLRQVector, trackingSIQVector, trackingAPQVector,
		trackingLateralQVector, trackingInDepthQVector,
		priorLRQVector, priorSIQVector, priorAPQVector,
		priorLateralQVector, priorInDepthQVector;
	for (unsigned int k = start; k <= end; ++k)
	{
		plotFrameNoQVector.push_back(m_PlotFrameNoQVector[k]);
		trackingLRQVector.push_back(m_TrackingLRQVector[k]);
		trackingSIQVector.push_back(m_TrackingSIQVector[k]);
		trackingAPQVector.push_back(m_TrackingAPQVector[k]);
		trackingLateralQVector.push_back(m_TrackingLateralQVector[k]);
		trackingInDepthQVector.push_back(m_TrackingInDepthQVector[k]);
		priorLRQVector.push_back(m_PriorLRQVector[k]);
		priorSIQVector.push_back(m_PriorSIQVector[k]);
		priorAPQVector.push_back(m_PriorAPQVector[k]);
		priorLateralQVector.push_back(m_PriorLateralQVector[k]);
		priorInDepthQVector.push_back(m_PriorInDepthQVector[k]);
	}

	ui->lrPlot->graph(0)->setData(plotFrameNoQVector, trackingLRQVector);
	ui->siPlot->graph(0)->setData(plotFrameNoQVector, trackingSIQVector);
	ui->apPlot->graph(0)->setData(plotFrameNoQVector, trackingAPQVector);
	ui->inPlanePlot->graph(0)->setData(plotFrameNoQVector, trackingLateralQVector);
	ui->inDepthPlot->graph(0)->setData(plotFrameNoQVector, trackingInDepthQVector);
	ui->lrPlot->graph(1)->setData(plotFrameNoQVector, priorLRQVector);
	ui->siPlot->graph(1)->setData(plotFrameNoQVector, priorSIQVector);
	ui->apPlot->graph(1)->setData(plotFrameNoQVector, priorAPQVector);
	ui->inPlanePlot->graph(1)->setData(plotFrameNoQVector, priorLateralQVector);
	ui->inDepthPlot->graph(1)->setData(plotFrameNoQVector, priorInDepthQVector);

	QVector<double> gtFrameNoQVector, gtLRQVector, gtSIQVector, gtAPQVector,
		gtLateralQVector, gtInDepthQVector;
	/// If ground truth is available
	if (ui->gtFileOKLabel->text().contains("OK"))
	{
		for (unsigned int k = 0; k != m_GTFrameNoQVector.size(); ++k)
		{
			if (m_GTFrameNoQVector[k] >= m_PlotFrameNoQVector[start] &&
				m_GTFrameNoQVector[k] <= m_PlotFrameNoQVector[end])
			{
				gtFrameNoQVector.push_back(m_GTFrameNoQVector[k]);
				gtLRQVector.push_back(m_GTLRQVector[k]);
				gtSIQVector.push_back(m_GTSIQVector[k]);
				gtAPQVector.push_back(m_GTAPQVector[k]);
				gtLateralQVector.push_back(m_GTLateralQVector[k]);
				gtInDepthQVector.push_back(m_GTInDepthQVector[k]);
			}
		}
		ui->lrPlot->graph(2)->setData(gtFrameNoQVector, gtLRQVector);
		ui->siPlot->graph(2)->setData(gtFrameNoQVector, gtSIQVector);
		ui->apPlot->graph(2)->setData(gtFrameNoQVector, gtAPQVector);
		ui->inPlanePlot->graph(2)->setData(gtFrameNoQVector, gtLateralQVector);
		ui->inDepthPlot->graph(2)->setData(gtFrameNoQVector, gtInDepthQVector);
	}

	ui->lrPlot->rescaleAxes();
	ui->siPlot->rescaleAxes();
	ui->apPlot->rescaleAxes();
	ui->inPlanePlot->rescaleAxes();
	ui->inDepthPlot->rescaleAxes();

	ui->lrPlot->replot();
	ui->siPlot->replot();
	ui->apPlot->replot();
	ui->inPlanePlot->replot();
	ui->inDepthPlot->replot();
}

void InVivoTrackingOffline::writeResults(unsigned int k_frame, unsigned int currentBin, double angle)
{
	unsigned int k_count = (k_frame - ui->startSpineBox->value() + 1) / ui->stepSpineBox->value();

	while (!m_TrackingResultsFile->open(QIODevice::Append | QIODevice::Text))
	{
		// DO NOTHING. Just wait till we can open it
	}

	// First look for ground truth entry that matches this frame
	double gtLR = 0, gtSI = 0, gtAP = 0, gtLateral = 0, gtInDepth = 0;
	if (m_GTFrameNoQVector.size() > 0)
	{
		for (unsigned int k = 0; k != m_GTFrameNoQVector.size(); ++k)
		{
			if (m_GTFrameNoQVector[k] == k_frame + 1)
			{
				gtLR = m_GTLRQVector[k];
				gtSI = m_GTSIQVector[k];
				gtAP = m_GTAPQVector[k];
				gtLateral = m_GTLateralQVector[k];
				gtInDepth = m_GTInDepthQVector[k];
				break;
			}
		}
	}

	QTextStream saveStream(m_TrackingResultsFile);

	saveStream
		<< QString::number(k_frame + 1) + "," // Frame Number
		<< QString::number(currentBin) + "," // Bin Number
		<< QString::number(angle) + "," // Mean Angle
		<< QString::number(m_TrackingLRQVector[k_count]) + ","// Tracking LR
		<< QString::number(m_TrackingSIQVector[k_count]) + ","// Tracking SI
		<< QString::number(m_TrackingAPQVector[k_count]) + ","// Tracking AP
		<< QString::number(m_TrackingLateralQVector[k_count]) + ","// Tracking Lateral
		<< QString::number(m_TrackingInDepthQVector[k_count]) + ","// Tracking In-depth
		<< QString::number(m_PriorLRQVector[k_count]) + ","// Prior LR
		<< QString::number(m_PriorSIQVector[k_count]) + ","// Prior SI
		<< QString::number(m_PriorAPQVector[k_count]) + ","// Prior AP
		<< QString::number(m_PriorLateralQVector[k_count]) + ","// Prior Lateral
		<< QString::number(m_PriorInDepthQVector[k_count]) + ","// Prior In-depth
		<< QString::number(gtLR) + ","// Ground Truth LR
		<< QString::number(gtSI) + ","// Ground Truth SI
		<< QString::number(gtAP) + ","// Ground Truth AP
		<< QString::number(gtLateral) + ","// Ground Truth Lateral
		<< QString::number(gtInDepth) + ","// Ground Truth In-depth
		<< QString::number(m_StateCovarianceMatrix(0, 0)) + ","// State Covariance 11
		<< QString::number(m_StateCovarianceMatrix(0, 1)) + ","// State Covariance 12
		<< QString::number(m_StateCovarianceMatrix(0, 2)) + ","// State Covariance 13
		<< QString::number(m_StateCovarianceMatrix(1, 1)) + ","// State Covariance 22
		<< QString::number(m_StateCovarianceMatrix(1, 2)) + ","// State Covariance 23
		<< QString::number(m_StateCovarianceMatrix(2, 2)) + ","// State Covariance 33
		<< QString::number(m_MatchingMetricValues[k_count]) + ","// Matching mismatch
		<< QString::number(m_BGMatchScoreValues[k_count]) + ","// BG subtraction residual
		<< QString::number(m_TargetTracker->GetBG2DShifts()[k_count][0]) + ","// BG 2D shift u
		<< QString::number(m_TargetTracker->GetBG2DShifts()[k_count][1])// BG 2D shift v
		<< endl;

	m_TrackingResultsFile->close();
}

// Set projection view pixmap
void InVivoTrackingOffline::setProjViewPixmap()
{
	// First, get by how much the target mask needs to be translated by
	double offsetX = m_TargetTracker->GetProjTranslationVector().back()[0] / m_TargetTracker->GetTargetDRRImage()->GetSpacing()[0];
	int shiftX = (offsetX > 0)? std::floor(offsetX + 0.5) : std::ceil(offsetX - 0.5)
		+ m_TargetTracker->GetTargetDRRROI().GetIndex()[0];
	double offsetY = m_TargetTracker->GetProjTranslationVector().back()[1] / m_TargetTracker->GetTargetDRRImage()->GetSpacing()[1];
	int shiftY = (offsetY > 0)? std::floor(offsetY + 0.5) : std::ceil(offsetY - 0.5)
		+ m_TargetTracker->GetTargetDRRROI().GetIndex()[1];

	QImage *projViewQImage = new QImage(m_TargetTracker->GetProjImage()->GetLargestPossibleRegion().GetSize()[0],
		m_TargetTracker->GetProjImage()->GetLargestPossibleRegion().GetSize()[1],
		QImage::Format_Grayscale8);

	ImageStatisticsType::Pointer projStats = ImageStatisticsType::New();
	projStats->SetInput(m_TargetTracker->GetProjImage());
	projStats->Update();
	m_ProjViewMin = projStats->GetMinimum();
	m_ProjViewMax = projStats->GetMaximum();

	ITKToQImageType::GenerateMonoScaleQImage(m_TargetTracker->GetProjImage(), projViewQImage, 
		m_TargetTracker->GetProjImage()->GetLargestPossibleRegion(), m_ProjViewMin, m_ProjViewMax);

	// Get the contour of the binary mask of the DRR image
	BinaryThresholdImageType::Pointer btFilter = BinaryThresholdImageType::New();
	btFilter->SetInput(m_TargetTracker->GetTargetDRRImage());
	btFilter->SetLowerThreshold(0);
	btFilter->SetUpperThreshold(0);
	btFilter->SetInsideValue(0);
	btFilter->SetOutsideValue(1);
	btFilter->Update();
	BinaryContourImageType::Pointer bcFilter = BinaryContourImageType::New();
	bcFilter->SetInput(btFilter->GetOutput());
	bcFilter->SetBackgroundValue(0);
	bcFilter->SetForegroundValue(1);
	bcFilter->SetFullyConnected(true);
	bcFilter->Update();
	StructuringElementType::RadiusType elementRadius;
	elementRadius.Fill(std::floor(m_TargetTracker->GetProjImage()->GetLargestPossibleRegion().GetSize()[0] * 0.0025 + 0.5));
	elementRadius[2] = 0;
	StructuringElementType structuringElement = StructuringElementType::Box(elementRadius);
	DilateImageType::Pointer dilateFilter = DilateImageType::New();
	dilateFilter->SetInput(bcFilter->GetOutput());
	dilateFilter->SetKernel(structuringElement);
	dilateFilter->SetDilateValue(1);
	dilateFilter->Update();

	QImage *projViewMaskQImage = new QImage(dilateFilter->GetOutput()->GetLargestPossibleRegion().GetSize()[0],
		dilateFilter->GetOutput()->GetLargestPossibleRegion().GetSize()[1],
		QImage::Format_Indexed8);
	ITKToQImageType::GenerateBinaryMaskQImage(dilateFilter->GetOutput(), projViewMaskQImage,
		dilateFilter->GetOutput()->GetLargestPossibleRegion(), 0, false);
	projViewMaskQImage->setColorTable(m_TrackingMaskColorTable);

	QPixmap* projViewPixmap = new QPixmap(projViewQImage->size());
	QPainter projViewPainter(projViewPixmap);
	projViewPainter.drawImage(0, 0, *projViewQImage);
	projViewPainter.drawImage(shiftX, shiftY, *projViewMaskQImage);

	QSize projLabelSize = ui->projViewLabel->size();
	QSize projImgSize = projViewPixmap->size();
	double projImgAspectRatio = double(projImgSize.height()) / double(projImgSize.width());
	if ((double(projLabelSize.height()) / double(projLabelSize.width())) >= projImgAspectRatio)
	{
		projImgSize.setWidth(projLabelSize.width());
		projImgSize.setHeight(projLabelSize.width() * projImgAspectRatio);
	}
	else if ((double(projLabelSize.height()) / double(projLabelSize.width())) < projImgAspectRatio)
	{
		projImgSize.setHeight(projLabelSize.height());
		projImgSize.setWidth(projLabelSize.height() / projImgAspectRatio);
	}
	ui->projViewLabel->setPixmap(projViewPixmap->scaled(projImgSize));

	// Save view image
	if (ui->writeResultsCheckBox->isChecked())
		projViewPixmap->scaled(projImgSize).save(
		QDir(QDir(ui->logDirTextEdit->toPlainText()).filePath(m_DateTime + "_ProjViews")).filePath(
		"Frame" + QString("%1").arg(m_TargetTracker->GetFrameNumbers().back(), 5, 10, QChar('0')) + ".png"), 0, 0);

	if (m_TargetTracker->GetUseBGSubtraction())
	{
		QImage *diffProjViewQImage = new QImage(m_TargetTracker->GetDiffProjImage()->GetLargestPossibleRegion().GetSize()[0],
			m_TargetTracker->GetDiffProjImage()->GetLargestPossibleRegion().GetSize()[1],
			QImage::Format_Grayscale8);

		ImageStatisticsType::Pointer diffProjStats = ImageStatisticsType::New();
		diffProjStats->SetInput(m_TargetTracker->GetDiffProjImage());
		diffProjStats->Update();
		m_DiffViewMin = diffProjStats->GetMinimum();
		m_DiffViewMax = diffProjStats->GetMaximum();

		ITKToQImageType::GenerateMonoScaleQImage(m_TargetTracker->GetDiffProjImage(), diffProjViewQImage,
			m_TargetTracker->GetDiffProjImage()->GetLargestPossibleRegion(), m_DiffViewMin, m_DiffViewMax);

		QPixmap* diffProjViewPixmap = new QPixmap(diffProjViewQImage->size());
		QPainter diffProjViewPainter(diffProjViewPixmap);
		diffProjViewPainter.drawImage(0, 0, *diffProjViewQImage);
		diffProjViewPainter.drawImage(shiftX, shiftY, *projViewMaskQImage);

		QSize diffProjLabelSize = ui->diffProjViewLabel->size();
		QSize diffProjImgSize = diffProjViewPixmap->size();
		double diffProjImgAspectRatio = double(diffProjImgSize.height()) / double(diffProjImgSize.width());
		if ((double(diffProjLabelSize.height()) / double(diffProjLabelSize.width())) >= diffProjImgAspectRatio)
		{
			diffProjImgSize.setWidth(diffProjLabelSize.width());
			diffProjImgSize.setHeight(diffProjLabelSize.width() * diffProjImgAspectRatio);

		}
		else if ((double(diffProjLabelSize.height()) / double(diffProjLabelSize.width())) < diffProjImgAspectRatio)
		{
			diffProjImgSize.setHeight(diffProjLabelSize.height());
			diffProjImgSize.setWidth(diffProjLabelSize.height() / diffProjImgAspectRatio);
		}
		ui->diffProjViewLabel->setPixmap(diffProjViewPixmap->scaled(diffProjImgSize));

		// Save view image
		if (ui->writeResultsCheckBox->isChecked())
			diffProjViewPixmap->scaled(diffProjImgSize).save(
			QDir(QDir(ui->logDirTextEdit->toPlainText()).filePath(m_DateTime + "_DiffProjViews")).filePath(
			"Frame" + QString("%1").arg(m_TargetTracker->GetFrameNumbers().back(), 5, 10, QChar('0')) + ".png"), 0, 0);
	}
}

// Write data properties
void InVivoTrackingOffline::writeDataProperties()
{
	if (!ui->writeResultsCheckBox->isChecked())
		return;
	QString configFile = QDir(ui->logDirTextEdit->toPlainText()).filePath(
		m_DateTime + "_Configuration.txt");
	QString dataPropertiesFilePath = QDir(ui->logDirTextEdit->toPlainText()).filePath(
		m_DateTime + "_DataProperties.txt");
	QFile *dataPropertiesFile = new QFile(dataPropertiesFilePath);
	if (!dataPropertiesFile->open(QIODevice::ReadWrite | QIODevice::Text))
	{
		QMessageBox::critical(this, "ERROR", "Cannot save data properties text file.");
		return;
	}

	QTextStream saveStream(dataPropertiesFile);

	saveStream << "Number of projections: " << m_TargetTracker->GetNProj() << endl;
	saveStream << "Number of bins: " << m_TargetTracker->GetNBin() << endl;
	saveStream << "Model size: " << "[" << 
		m_TargetTracker->GetModelSize()[0] << ", " <<
		m_TargetTracker->GetModelSize()[1] << ", " << 
		m_TargetTracker->GetModelSize()[0] << "]" << endl;
	saveStream << "Model spacings (mm): " << "[" <<
		m_TargetTracker->GetModelSpacing()[0] << ", " <<
		m_TargetTracker->GetModelSpacing()[1] << ", " <<
		m_TargetTracker->GetModelSpacing()[0] << "]" << endl;
	saveStream << "Projection size: " << "[" <<
		m_TargetTracker->GetProjSize()[0] << ", " <<
		m_TargetTracker->GetProjSize()[1] << "]" << endl;
	saveStream << "Projection spacings (mm): " << "[" <<
		m_TargetTracker->GetProjSpacing()[0] << ", " <<
		m_TargetTracker->GetProjSpacing()[1] << "]" << endl;

	dataPropertiesFile->close();
}

void InVivoTrackingOffline::readSurrogateFile()
{
	m_TargetTracker->GetSurrogateSignal().clear();
	QString loadFile_QStr = ui->surrogateFileTextEdit->toPlainText();
	QFile loadFile(loadFile_QStr);
	if (!loadFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QMessageBox::critical(this, "ERROR", "Cannot load surrogate signal csv file.");
		ui->gtFileOKLabel->setText("Not set");
		ui->gtFileOKLabel->setStyleSheet("QLabel{ color: black; }");
		return;
	}
	QTextStream loadStream(&loadFile);
	QStringList parts;
	QString line;

	line = loadStream.readLine();
	parts = line.split(",");
	for (unsigned int k = 0; k < 3; ++k)
		m_TargetTracker->SetSurrogacySlope(k, parts[k].trimmed().toDouble());

	line = loadStream.readLine();
	parts = line.split(",");
	for (unsigned int k = 0; k < 3; ++k)
		m_TargetTracker->SetSurrogacyOffset(k, parts[k].trimmed().toDouble());

	line = loadStream.readLine();
	while (!line.isNull())
	{
		m_TargetTracker->GetSurrogateSignal().push_back(line.trimmed().toDouble());
		line = loadStream.readLine();
	}
	loadFile.close();
}

void InVivoTrackingOffline::readGeometryFile()
{
	m_GeometryReader = rtk::ThreeDCircularProjectionGeometryXMLFileReader::New();
	m_GeometryReader->SetFilename(ui->geometryFileTextEdit->toPlainText().toStdString());
	try
	{
		m_GeometryReader->GenerateOutputInformation();
	}
	catch (itk::ExceptionObject)
	{
		QMessageBox::critical(this, "ERROR", "Cannot read geometry file.");
		return;
	}
	m_GeometryReader->GetOutputObject()->SetShortScanOff();
	m_GeometryReader->GetOutputObject()->SetDTSOn();
	if (m_GeometryReader->GetOutputObject()->GetGantryAngles().size()
		!= m_TargetTracker->GetNProj())
	{
		QMessageBox::critical(this, "ERROR", "Geometry file does not contain the same number of entries as the number of projections.");
		return;
	}
}

void InVivoTrackingOffline::getGroundTruthLateralInDepth()
{
	m_GTLateralQVector.clear();
	m_GTInDepthQVector.clear();
	if (ui->gtFileOKLabel->text().contains("OK"))
	{
		for (unsigned int k = 0; k != m_GTFrameNoQVector.size(); ++k)
		{
			VectorType gtPos(3, 0);
			gtPos(0) = m_GTLRQVector[k];
			gtPos(1) = m_GTSIQVector[k];
			gtPos(2) = m_GTAPQVector[k];
			VectorType gtProjPos = m_GeometryReader->GetOutputObject()->CalculateProjSpaceCoordinate(gtPos, m_GTFrameNoQVector[k] - 1);
			m_GTLateralQVector.push_back(gtProjPos(0));
			m_GTInDepthQVector.push_back(gtProjPos(2));
		}
	}
	ui->inPlanePlot->graph(0)->removeDataAfter(0);
	ui->inPlanePlot->graph(1)->removeDataAfter(0);
	ui->inDepthPlot->graph(0)->removeDataAfter(0);
	ui->inDepthPlot->graph(1)->removeDataAfter(0);
	ui->inPlanePlot->graph(2)->setData(m_GTFrameNoQVector, m_GTLateralQVector);
	ui->inDepthPlot->graph(2)->setData(m_GTFrameNoQVector, m_GTInDepthQVector);
	ui->inPlanePlot->rescaleAxes();
	ui->inDepthPlot->rescaleAxes();
	ui->inPlanePlot->replot();
	ui->inDepthPlot->replot();
}

void InVivoTrackingOffline::clearResults()
{
	m_PlotFrameNoQVector.clear();
	m_TrackingLRQVector.clear();
	m_TrackingSIQVector.clear();
	m_TrackingAPQVector.clear();
	m_TrackingLateralQVector.clear();
	m_TrackingInDepthQVector.clear();
	m_PriorLRQVector.clear();
	m_PriorSIQVector.clear();
	m_PriorAPQVector.clear();
	m_PriorLateralQVector.clear();
	m_PriorInDepthQVector.clear();
	m_MatchingMetricValues.clear();
	m_BGMatchScoreValues.clear();
}

void InVivoTrackingOffline::logFilesBeforeStart()
{
	m_DateTime =
		QString("%1").arg(QDateTime::currentDateTime().date().year(), 4, 10, QChar('0')) + "-" +
		QString("%1").arg(QDateTime::currentDateTime().date().month(), 2, 10, QChar('0')) + "-" +
		QString("%1").arg(QDateTime::currentDateTime().date().day(), 2, 10, QChar('0')) + "-" +
		QString("%1").arg(QDateTime::currentDateTime().time().hour(), 2, 10, QChar('0')) + "-" +
		QString("%1").arg(QDateTime::currentDateTime().time().minute(), 2, 10, QChar('0')) + "-" +
		QString("%1").arg(QDateTime::currentDateTime().time().second(), 2, 10, QChar('0'));

	QString configFile = QDir(ui->logDirTextEdit->toPlainText()).filePath(
		m_DateTime + "_Configuration.txt");
	this->saveConfiguration(configFile);

	// Initialize results logging
	QString resultFilePath = QDir(ui->logDirTextEdit->toPlainText()).filePath(
		m_DateTime + "_TrackingResults.csv");
	m_TrackingResultsFile = new QFile(resultFilePath);
	if (!m_TrackingResultsFile->open(QIODevice::ReadWrite | QIODevice::Text))
	{
		QMessageBox::critical(this, "ERROR", "Cannot write to tracking result file.");
		this->enableInputFields();
		this->checkStartRequirement();
		ui->programStatusLabel->setText("Idle");
		ui->programStatusLabel->setStyleSheet("QLabel{ color: blue; }");
		return;
	}
	QTextStream saveStream(m_TrackingResultsFile);
	saveStream
		<< "Frame Number,"
		<< "Bin Number,"
		<< "Angle,"
		<< "Tracking LR,"
		<< "Tracking SI,"
		<< "Tracking AP,"
		<< "Tracking Lateral,"
		<< "Tracking In-depth,"
		<< "Prior LR,"
		<< "Prior SI,"
		<< "Prior AP,"
		<< "Prior Lateral,"
		<< "Prior In-depth,"
		<< "Ground Truth LR,"
		<< "Ground Truth SI,"
		<< "Ground Truth AP,"
		<< "Ground Truth Lateral,"
		<< "Ground Truth In-depth,"
		<< "State Covariance 11,"
		<< "State Covariance 12,"
		<< "State Covariance 13,"
		<< "State Covariance 22,"
		<< "State Covariance 23,"
		<< "State Covariance 33,"
		<< "Template Matching Metric,"
		<< "Background Matching Score,"
		<< "BG Shift U,"
		<< "BG Shift V"
		<< endl;
	m_TrackingResultsFile->close();

	// Also create folder for saving proj and diff projection views
	QString projViewFolder = QDir(ui->logDirTextEdit->toPlainText()).filePath(
		m_DateTime + "_ProjViews");
	if (!QDir(projViewFolder).exists())
		QDir().mkdir(projViewFolder);

	if (m_TargetTracker->GetUseBGSubtraction())
	{
		QString diffProjViewFolder = QDir(ui->logDirTextEdit->toPlainText()).filePath(
			m_DateTime + "_DiffProjViews");
		if (!QDir(diffProjViewFolder).exists())
			QDir().mkdir(diffProjViewFolder);
	}
}

void InVivoTrackingOffline::getGroundTruthLRSIAP()
{
	QString loadFile_QStr = ui->gtFileTextEdit->toPlainText();
	QFile loadFile(loadFile_QStr);
	if (!loadFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QMessageBox::critical(this, "ERROR", "Cannot load ground truth csv file.");
		ui->gtFileOKLabel->setText("Not set");
		ui->gtFileOKLabel->setStyleSheet("QLabel{ color: black; }");
		return;
	}
	QTextStream loadStream(&loadFile);
	QString line = loadStream.readLine();
	while (!line.isNull())
	{
		QStringList parts;
		parts = line.split(",");
		if (parts.size() < 4)
		{
			QMessageBox::critical(this, "ERROR", "Incompatible ground truth csv file format.");
			ui->gtFileOKLabel->setText("Not set");
			ui->gtFileOKLabel->setStyleSheet("QLabel{ color: black; }");
			return;
		}
		m_GTFrameNoQVector.push_back(parts[0].trimmed().toDouble());
		m_GTLRQVector.push_back(parts[1].trimmed().toDouble());
		m_GTSIQVector.push_back(parts[2].trimmed().toDouble());
		m_GTAPQVector.push_back(parts[3].trimmed().toDouble());
		line = loadStream.readLine();
	}
	loadFile.close();

	ui->lrPlot->graph(0)->removeDataAfter(0);
	ui->lrPlot->graph(1)->removeDataAfter(0);
	ui->siPlot->graph(0)->removeDataAfter(0);
	ui->siPlot->graph(1)->removeDataAfter(0);
	ui->apPlot->graph(0)->removeDataAfter(0);
	ui->apPlot->graph(1)->removeDataAfter(0);
	ui->lrPlot->graph(2)->setData(m_GTFrameNoQVector, m_GTLRQVector);
	ui->siPlot->graph(2)->setData(m_GTFrameNoQVector, m_GTSIQVector);
	ui->apPlot->graph(2)->setData(m_GTFrameNoQVector, m_GTAPQVector);
	ui->lrPlot->rescaleAxes();
	ui->siPlot->rescaleAxes();
	ui->apPlot->rescaleAxes();
	ui->lrPlot->replot();
	ui->siPlot->replot();
	ui->apPlot->replot();

	// Pass ground truth to tracking filter for backdoor checking purpose
	std::vector<VectorType> groundTruthTrj;
	std::vector<unsigned int> groundTruthFrames;
	for (unsigned int k = 0; k != m_GTFrameNoQVector.size(); ++k)
	{
		groundTruthFrames.push_back(m_GTFrameNoQVector[k]-1);
		VectorType groundTruthPos(3,0);
		groundTruthPos[0] = m_GTLRQVector[k];
		groundTruthPos[1] = -m_GTSIQVector[k];
		groundTruthPos[2] = m_GTAPQVector[k];
		groundTruthTrj.push_back(groundTruthPos);
	}
	m_TargetTracker->SetGroundTruthFrames(groundTruthFrames);
	m_TargetTracker->SetGroundTruthTrj(groundTruthTrj);
}