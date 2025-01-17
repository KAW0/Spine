/*
	This file is part of Spine.

    Spine is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Spine is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Spine.  If not, see <http://www.gnu.org/licenses/>.
 */
// Copyright 2018 Clockwork Origins

#include "widgets/ModInfoPage.h"

#include "IconCache.h"
#include "InstallMode.h"
#include "SpineConfig.h"

#include "common/GameType.h"
#include "common/SpineModules.h"

#include "gui/DownloadQueueWidget.h"
#include "gui/FullscreenPreview.h"
#include "gui/ReportContentDialog.h"
#include "gui/Spoiler.h"
#include "gui/WaitSpinner.h"

#include "https/Https.h"

#include "utils/Compression.h"
#include "utils/Config.h"
#include "utils/Conversion.h"
#include "utils/Database.h"
#include "utils/FileDownloader.h"
#include "utils/Hashing.h"
#include "utils/MultiFileDownloader.h"

#include "widgets/FeedbackDialog.h"
#include "widgets/NewsWidget.h"
#include "widgets/ProjectInfoBoxWidget.h"
#include "widgets/RatingWidget.h"
#include "widgets/ReviewWidget.h"
#include "widgets/UpdateLanguage.h"

#include "clockUtils/sockets/TcpSocket.h"

#include <QApplication>
#include <QCheckBox>
#include <QDirIterator>
#include <QFileDialog>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QGroupBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMessageBox>
#include <QMouseEvent>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QStandardItemModel>
#include <QScrollArea>
#include <QSvgWidget>
#include <QtConcurrentRun>
#include <QTextBrowser>
#include <QVBoxLayout>

using namespace spine::client;
using namespace spine::gui;
using namespace spine::https;
using namespace spine::utils;
using namespace spine::widgets;

ModInfoPage::ModInfoPage(QMainWindow * mainWindow, QWidget * par) : QWidget(par), _mainWindow(mainWindow), _projectNameLabel(nullptr), _previewImageLabel(nullptr), _ratingWidget(nullptr), _rateWidget(nullptr), _thumbnailView(nullptr), _installButton(nullptr), _descriptionView(nullptr), _spineFeaturesView(nullptr), _thumbnailModel(nullptr), _spineFeatureModel(nullptr), _projectID(-1), _editInfoPageButton(nullptr), _descriptionEdit(nullptr), _featuresEdit(nullptr), _spineFeaturesEdit(nullptr), _addImageButton(nullptr), _deleteImageButton(nullptr), _applyButton(nullptr), _waitSpinner(nullptr), _forceEdit(false) {
	auto * l = new QVBoxLayout();
	l->setAlignment(Qt::AlignTop);

	_scrollArea = new QScrollArea(this);
	auto * widget = new QWidget(_scrollArea);
	auto * scrollLayout = new QVBoxLayout();
	scrollLayout->setAlignment(Qt::AlignTop);
	widget->setLayout(scrollLayout);
	_scrollArea->setWidget(widget);
	_scrollArea->setWidgetResizable(true);
	_scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	widget->setProperty("achievementEditor", true);

	_projectNameLabel = new QLabel(widget);
	_projectNameLabel->setProperty("modnameTitle", true);
	scrollLayout->addWidget(_projectNameLabel, 0, Qt::AlignTop);
	_projectNameLabel->hide();

	{
		auto * hl = new QHBoxLayout();
		hl->addStretch(25);
		_previewImageLabel = new QLabel(widget);
		hl->addWidget(_previewImageLabel, 50, Qt::AlignHCenter);
		_previewImageLabel->hide();
		_previewImageLabel->setFixedSize(640, 480);

		{
			auto * vl = new QVBoxLayout();
			_ratingWidget = new RatingWidget(RatingWidget::RatingMode::Overall, this);
			vl->addWidget(_ratingWidget, 0, Qt::AlignTop | Qt::AlignRight);
			_ratingWidget->setEditable(false);
			_ratingWidget->setVisible(false);

			_rateWidget = new RatingWidget(RatingWidget::RatingMode::User, this);
			vl->addWidget(_rateWidget, 0, Qt::AlignTop | Qt::AlignRight);
			_rateWidget->setEditable(true);
			_rateWidget->setVisible(false);

			connect(_rateWidget, &RatingWidget::editReview, this, &ModInfoPage::editReview);

			vl->addStretch(1);

			_projectInfoBoxWidget = new ProjectInfoBoxWidget(this);
			vl->addWidget(_projectInfoBoxWidget, 0, Qt::AlignBottom | Qt::AlignRight);
			
			hl->addLayout(vl, 25);
		}

		auto * vl = new QVBoxLayout();
		_addImageButton = new QPushButton(IconCache::getInstance()->getOrLoadIcon(":/svg/add.svg"), "", widget);
		connect(_addImageButton, &QPushButton::released, this, &ModInfoPage::addImage);
		_addImageButton->hide();

		vl->addWidget(_addImageButton, 0, Qt::AlignTop | Qt::AlignRight);

		_deleteImageButton = new QPushButton(IconCache::getInstance()->getOrLoadIcon(":/svg/remove.svg"), "", widget);
		connect(_deleteImageButton, &QPushButton::released, this, &ModInfoPage::removeImage);
		_deleteImageButton->hide();

		vl->addWidget(_deleteImageButton, 0, Qt::AlignTop | Qt::AlignRight);
		vl->addStretch(1);

		hl->addLayout(vl);
		hl->setStretchFactor(_previewImageLabel, 1);

		scrollLayout->addLayout(hl);
	}

	_thumbnailView = new QListView(widget);
	_thumbnailView->setIconSize(QSize(300, 100));
	_thumbnailView->setViewMode(QListView::IconMode);
	_thumbnailView->setUniformItemSizes(true);
	scrollLayout->addWidget(_thumbnailView, 0, Qt::AlignCenter);
	_thumbnailView->hide();
	_thumbnailView->setFixedHeight(140);
	_thumbnailView->setSpacing(5);
	_thumbnailView->setHorizontalScrollMode(QAbstractItemView::ScrollMode::ScrollPerPixel);
	_thumbnailView->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
	_thumbnailView->setWrapping(false);
	connect(_thumbnailView, &QListView::clicked, this, &ModInfoPage::changePreviewImage);
	connect(_thumbnailView, &QListView::doubleClicked, this, &ModInfoPage::changePreviewImage);
	connect(_thumbnailView, &QListView::doubleClicked, this, &ModInfoPage::showFullscreen);
	connect(_thumbnailView, &QListView::activated, this, &ModInfoPage::showFullscreen);

	_thumbnailModel = new QStandardItemModel(_thumbnailView);
	_thumbnailView->setModel(_thumbnailModel);
	connect(_thumbnailView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ModInfoPage::changedThumbnailSelection);

	_installButton = new QPushButton(IconCache::getInstance()->getOrLoadIcon(":/svg/download.svg"), QApplication::tr("Install"), widget);
	scrollLayout->addWidget(_installButton, 0, Qt::AlignLeft);
	_installButton->hide();
	UPDATELANGUAGESETTEXT(_installButton, "Install");
	connect(_installButton, &QPushButton::released, this, &ModInfoPage::installProject);

	_startButton = new QPushButton(QApplication::tr("StartMod"), widget);
	scrollLayout->addWidget(_startButton, 0, Qt::AlignLeft);
	_startButton->hide();
	UPDATELANGUAGESETTEXT(_startButton, "StartMod");
	connect(_startButton, &QPushButton::released, this, &ModInfoPage::startProject);

	_optionalPackageButtonsLayout = new QVBoxLayout();
	scrollLayout->addLayout(_optionalPackageButtonsLayout);

	auto * hl = new QHBoxLayout();
	hl->setAlignment(Qt::AlignTop);

	_descriptionView = new QTextBrowser(widget);
	_descriptionView->setOpenExternalLinks(true);
	hl->addWidget(_descriptionView, 75);
	_descriptionView->hide();

	_descriptionEdit = new QTextEdit(widget);
	hl->addWidget(_descriptionEdit, 75);
	_descriptionEdit->hide();
	_descriptionEdit->setPlaceholderText(QApplication::tr("InfoPageDescriptionPlaceholder"));
	UPDATELANGUAGESETPLACEHOLDERTEXT(_descriptionEdit, "InfoPageDescriptionPlaceholder");

	{
		auto * rightLayout = new QVBoxLayout();
		rightLayout->setAlignment(Qt::AlignTop);
		
		_spineFeaturesView = new QListView(widget);
		_spineFeaturesView->hide();

		_spineFeaturesEdit = new QGroupBox(QApplication::tr("SpineFeatures"), widget);
		_spineFeaturesEdit->hide();
		UPDATELANGUAGESETTITLE(_spineFeaturesEdit, "SpineFeatures");

		rightLayout->addWidget(_spineFeaturesView);
		rightLayout->addWidget(_spineFeaturesEdit);

		_historyBox = new QGroupBox(QApplication::tr("History"), this);
		UPDATELANGUAGESETTITLE(_historyBox, "History");

		_historyLayout = new QVBoxLayout();
		_historyLayout->setAlignment(Qt::AlignTop);

		_historyBox->setLayout(_historyLayout);

		rightLayout->addWidget(_historyBox);

		{
			_feedbackButton = new QPushButton(QApplication::tr("Feedback"), widget);
			UPDATELANGUAGESETTEXT(_feedbackButton, "Feedback");
			_feedbackButton->setProperty("library", true);
			_feedbackButton->hide();
			connect(_feedbackButton, &QPushButton::released, this, &ModInfoPage::feedbackClicked);

			rightLayout->addWidget(_feedbackButton);
		}

		hl->addLayout(rightLayout, 25);
	}
	{
		auto * vl = new QVBoxLayout();
		vl->setAlignment(Qt::AlignTop);
		_spineFeaturesEdit->setLayout(vl);

		{
			auto * cb = new QCheckBox(QApplication::tr("AchievementModule"), _spineFeaturesEdit);
			_moduleCheckBoxes.insert(common::SpineModules::Achievements, cb);
			vl->addWidget(cb);
		}
		{
			auto * cb = new QCheckBox(QApplication::tr("ScoresModule"), _spineFeaturesEdit);
			_moduleCheckBoxes.insert(common::SpineModules::Scores, cb);
			vl->addWidget(cb);
		}
		{
			auto * cb = new QCheckBox(QApplication::tr("MultiplayerModule"), _spineFeaturesEdit);
			_moduleCheckBoxes.insert(common::SpineModules::Multiplayer, cb);
			cb->setVisible(false); // can't be changed by users as it gets abused
			vl->addWidget(cb);
		}
		{
			auto * cb = new QCheckBox(QApplication::tr("OverallSaveModule"), _spineFeaturesEdit);
			_moduleCheckBoxes.insert(common::SpineModules::OverallSave, cb);
			vl->addWidget(cb);
		}
		{
			auto * cb = new QCheckBox(QApplication::tr("GamepadModule"), _spineFeaturesEdit);
			_moduleCheckBoxes.insert(common::SpineModules::Gamepad, cb);
			vl->addWidget(cb);
		}
		{
			auto * cb = new QCheckBox(QApplication::tr("FriendsModule"), _spineFeaturesEdit);
			_moduleCheckBoxes.insert(common::SpineModules::Friends, cb);
			vl->addWidget(cb);
		}
		{
			auto * cb = new QCheckBox(QApplication::tr("StatisticsModule"), _spineFeaturesEdit);
			_moduleCheckBoxes.insert(common::SpineModules::Statistics, cb);
			vl->addWidget(cb);
		}
	}

	hl->setStretchFactor(_descriptionView, 75);
	hl->setStretchFactor(_descriptionEdit, 75);

	_spineFeatureModel = new QStandardItemModel(_spineFeaturesView);
	_spineFeaturesView->setModel(_spineFeatureModel);
	_spineFeaturesView->setEditTriggers(QAbstractItemView::EditTrigger::NoEditTriggers);
	_spineFeaturesView->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectItems);
	_spineFeaturesView->setSelectionMode(QAbstractItemView::SelectionMode::NoSelection);
	connect(_spineFeaturesView, &QListView::clicked, this, &ModInfoPage::selectedSpineFeature);

	scrollLayout->addLayout(hl);

	_featuresEdit = new QLineEdit(widget);
	scrollLayout->addWidget(_featuresEdit);
	_featuresEdit->hide();
	_featuresEdit->setPlaceholderText(QApplication::tr("FeaturesEditPlaceholder"));
	UPDATELANGUAGESETPLACEHOLDERTEXT(_featuresEdit, "FeaturesEditPlaceholder");

	l->addWidget(_scrollArea);

	_ratingsBox = new QGroupBox(QApplication::tr("Ratings"), this);

	{
		auto * ratingsLayout = new QVBoxLayout();

		for (int i = 4; i >= 0; i--) {
			auto * horiLayout = new QHBoxLayout();
			for (int j = 0; j < 5; j++) {
				auto * svgWidget = new QSvgWidget(j <= i ? ":/svg/star-full.svg" : ":/svg/star.svg", _ratingsBox);
				svgWidget->setFixedSize(QSize(25, 25));
				horiLayout->addWidget(svgWidget);
			}

			Rating r {};
			r.shareView = new QProgressBar(_ratingsBox);
			r.shareView->setMinimum(0);
			r.shareView->setMaximum(1);
			r.shareView->setValue(0);
			r.shareView->setFormat("(%v)");
			r.shareView->setFixedWidth(300);
			r.shareView->setTextVisible(false);
			r.text = new QLabel(_ratingsBox);

			horiLayout->addWidget(r.shareView);
			horiLayout->addWidget(r.text);

			horiLayout->addStretch(1);

			_ratings.append(r);

			ratingsLayout->addLayout(horiLayout);
		}

		_ratingsBox->setLayout(ratingsLayout);
	}

	scrollLayout->addWidget(_ratingsBox);

	{
		_ownReviewWidget = new QWidget(_scrollArea);

		auto * vl = new QVBoxLayout();
		
		_ownReviewEdit = new QTextEdit(_ownReviewWidget);

		vl->addWidget(_ownReviewEdit);

		auto * pb = new QPushButton(QApplication::tr("Submit"), _ownReviewWidget);
		UPDATELANGUAGESETTEXT(pb, "Submit");

		vl->addWidget(pb, 0, Qt::AlignBottom | Qt::AlignRight);
		
		_ownReviewWidget->setVisible(false);

		_ownReviewWidget->setLayout(vl);

		scrollLayout->addWidget(_ownReviewWidget);

		connect(pb, &QPushButton::released, this, &ModInfoPage::submitReview);
	}

	_reviewLayout = new QVBoxLayout();

	scrollLayout->addLayout(_reviewLayout);

	{
		auto * hlBottom = new QHBoxLayout();

		hlBottom->addStretch(1);

		auto * reportContentBtn = new QPushButton(IconCache::getInstance()->getOrLoadIcon(":/svg/flag.svg"), "", this);
		reportContentBtn->setToolTip(QApplication::tr("ReportContent"));

		connect(reportContentBtn, &QPushButton::released, this, [this]() {
			ReportContentDialog dlg("InfoPage_" + QString::number(_projectID), this);
			dlg.exec();
		});

		hlBottom->addWidget(reportContentBtn);
		
		_editInfoPageButton = new QPushButton(IconCache::getInstance()->getOrLoadIcon(":/svg/edit.svg"), "", this);
		_editInfoPageButton->setToolTip(QApplication::tr("EditInfoPageTooltip"));
		UPDATELANGUAGESETTOOLTIP(_editInfoPageButton, "EditInfoPageTooltip");
		hlBottom->addWidget(_editInfoPageButton, 0, Qt::AlignBottom | Qt::AlignRight);
		_editInfoPageButton->hide();
		connect(_editInfoPageButton, &QPushButton::released, this, &ModInfoPage::switchToEdit);

		l->addLayout(hlBottom);
	}

	_applyButton = new QPushButton(QApplication::tr("Apply"), this);
	UPDATELANGUAGESETTEXT(_applyButton, "Apply");
	l->addWidget(_applyButton, 0, Qt::AlignBottom | Qt::AlignRight);
	_applyButton->hide();
	connect(_applyButton, &QPushButton::released, this, &ModInfoPage::submitChanges);

	scrollLayout->addStretch(1);

	setLayout(l);

	qRegisterMetaType<int32_t>("int32_t");
	connect(this, &ModInfoPage::receivedPage, this, &ModInfoPage::updatePage);
	connect(this, &ModInfoPage::gotRandomMod, this, &ModInfoPage::loadPage);
	connect(this, &ModInfoPage::receivedRatings, this, &ModInfoPage::updateRatings);
	connect(this, &ModInfoPage::receivedReviews, this, &ModInfoPage::updateReviews);
	connect(this, &ModInfoPage::reviewSubmitted, this, &ModInfoPage::submittedReview);
}

void ModInfoPage::loginChanged() {
	_rateWidget->loginChanged();
}

void ModInfoPage::loadPage(int32_t projectID) {
	if (projectID == -1) return;

	_ownReviewWidget->setVisible(false);
	
	delete _waitSpinner;
	_waitSpinner = new WaitSpinner(QApplication::tr("LoadingPage"), this);
	_projectID = projectID;

	for (auto * rw : _reviewWidgets) {
		rw->deleteLater();
	}
	_reviewWidgets.clear();

	{
		QJsonObject json;
		json["Username"] = Config::Username;
		json["Password"] = Config::Password;
		json["Language"] = Config::Language;
		json["ProjectID"] = projectID;

		Https::postAsync(DATABASESERVER_PORT, "requestInfoPage", QJsonDocument(json).toJson(QJsonDocument::Compact), [this](const QJsonObject & responseData, int responseCode) {
			if (responseCode != 200) return;

			emit receivedPage(responseData);
		});
	}

	{
		QJsonObject requestData;
		requestData["ProjectID"] = _projectID;

		Https::postAsync(DATABASESERVER_PORT, "getRatings", QJsonDocument(requestData).toJson(QJsonDocument::Compact), [this, projectID](const QJsonObject & json, int statusCode) {
			if (statusCode != 200) return;

			if (projectID != _projectID) return;

			if (!json.contains("Rating1")) return;

			if (!json.contains("Rating2")) return;

			if (!json.contains("Rating3")) return;

			if (!json.contains("Rating4")) return;

			if (!json.contains("Rating5")) return;

			const int rating1 = json["Rating1"].toString().toInt();
			const int rating2 = json["Rating2"].toString().toInt();
			const int rating3 = json["Rating3"].toString().toInt();
			const int rating4 = json["Rating4"].toString().toInt();
			const int rating5 = json["Rating5"].toString().toInt();

			emit receivedRatings(rating1, rating2, rating3, rating4, rating5);
		});

		Https::postAsync(DATABASESERVER_PORT, "getReviews", QJsonDocument(requestData).toJson(QJsonDocument::Compact), [this, projectID](const QJsonObject & json, int statusCode) {
			if (statusCode != 200) return;

			if (projectID != _projectID) return;

			if (!json.contains("Reviews")) return;

			emit receivedReviews(json["Reviews"].toArray());
		});
	}
}

void ModInfoPage::finishedInstallation(int projectID, int, bool success) {
	if (_projectID != projectID || !success) return;

	Database::DBError err;
	const bool installed = Database::queryCount(Config::BASEDIR.toStdString() + "/" + INSTALLED_DATABASE, "SELECT * FROM mods WHERE ModID = " + std::to_string(_projectID) + " LIMIT 1;", err) > 0;
	_installButton->setVisible(!installed);
	_startButton->setVisible(installed && !_runningUpdates.contains(_projectID));

	for (QPushButton * pb : _optionalPackageButtons) {
		const bool packageInstalled = Database::queryCount(Config::BASEDIR.toStdString() + "/" + INSTALLED_DATABASE, "SELECT PackageID FROM packages WHERE ModID = " + std::to_string(_projectID) + " AND PackageID = " + std::to_string(pb->property("packageid").toInt()) + " LIMIT 1;", err) > 0;
		pb->setVisible(installed && !packageInstalled);
	}
}

void ModInfoPage::updatePage(QJsonObject json) {
	const QString projectName = json["Name"].toString();
	const int majorVersion = json["MajorVersion"].toString().toInt();
	const int minorVersion = json["MinorVersion"].toString().toInt();
	const int patchVersion = json["PatchVersion"].toString().toInt();
	
	delete _waitSpinner;
	_waitSpinner = nullptr;
	_projectNameLabel->setText(projectName + QString(" (%1.%2.%3)").arg(majorVersion).arg(minorVersion).arg(patchVersion));
	_projectNameLabel->setVisible(!projectName.isEmpty());

	for (QPushButton * pb : _optionalPackageButtons) {
		pb->deleteLater();
	}
	_optionalPackageButtons.clear();

	for (QWidget * w : _historyWidgets) {
		w->deleteLater();
	}
	_historyWidgets.clear();

	_ratingWidget->setProjectID(_projectID);
	_rateWidget->setProjectID(_projectID);
	_ratingWidget->setModName(projectName);
	_rateWidget->setModName(projectName);
	_ratingWidget->setVisible(true);
	_rateWidget->setVisible(true);

	_projectInfoBoxWidget->update(json);

	_thumbnailModel->clear();
	_screens.clear();

	if (json.contains("Screenshots")) {
		for (auto jsonRef : json["Screenshots"].toArray()) {
			const auto jsonScreen = jsonRef.toObject();

			const auto file = jsonScreen["File"].toString();
			const auto hash = jsonScreen["Hash"].toString();

			_screens << qMakePair(file, hash);
		}
	}
	
	_previewImageLabel->setVisible(false);
	_thumbnailView->setVisible(false);
	
	if (!_screens.empty()) {
		auto empty = true;
			
		auto * mfd = new MultiFileDownloader(this);
		for (const auto & p : _screens) {
			QString filename = p.first;
			filename.chop(2); // every image is compressed, so it has a .z at the end

			const auto targetFile = Config::DOWNLOADDIR + "/screens/" + QString::number(_projectID) + "/" + filename;
			
			if (!QFileInfo::exists(targetFile) || !Hashing::checkHash(targetFile, p.second)) {
				empty = false;
				
				QFileInfo fi(p.first);
				auto * fd = new FileDownloader(QUrl("https://clockwork-origins.de/Gothic/downloads/mods/" + QString::number(_projectID) + "/screens/" + p.first), Config::DOWNLOADDIR + "/screens/" + QString::number(_projectID) + "/" + fi.path(), fi.fileName(), p.second, mfd);
				mfd->addFileDownloader(fd);
			}
		}

		connect(mfd, &MultiFileDownloader::downloadSucceeded, this, &ModInfoPage::showScreens);

		if (empty) {
			delete mfd;
			showScreens();
		} else {
			DownloadQueueWidget::getInstance()->addDownload(QApplication::tr("ScreensFor").arg(projectName), mfd);
		}
	} else {
		_previewImageLabel->setPixmap(QPixmap());
	}

	QString description = decodeString(json["Description"].toString());
	QString infoText = description;

	if (infoText.isEmpty()) {
		infoText = QApplication::tr("NoDescriptionAvailable");
		infoText += QString(" <a href=\"%1\">%1</a>").arg("https://forum.worldofplayers.de/forum/threads/1490886-Mod-Beschreibungen-f%C3%BCr-Spine-gesucht");
	}

	QString featuresText;
	if (json.contains("Features")) {
		infoText.append("<br><br><strong>" + QApplication::tr("Features") + ":</strong><br><ul>");
		for (auto jsonRef : json["Features"].toArray()) {
			const auto s = decodeString(jsonRef.toString());
			infoText.append("<li> " + s);
			featuresText.append(s + ";");
		}
		infoText.append("</ul>");
	}
	
	_descriptionView->setHtml(infoText);
	_descriptionView->setVisible(true);
	_descriptionView->setMinimumHeight(static_cast<int>(_descriptionView->document()->size().height() + 75));
	_descriptionView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
	_descriptionEdit->setPlainText(description);
	
	_featuresEdit->setText(featuresText.replace("&apos;", "'"));

	_spineFeatureModel->clear();

	const auto spineFeatures = json["SpineFeatures"].toString().toInt();
	
	if (spineFeatures & common::SpineModules::Achievements) {
		auto * itm = new QStandardItem(QApplication::tr("AchievementModule"));
		_spineFeatureModel->appendRow(itm);
		itm->setData(static_cast<int>(common::SpineModules::Achievements), Qt::UserRole);
	}
	if (spineFeatures & common::SpineModules::Scores) {
		auto * itm = new QStandardItem(QApplication::tr("ScoresModule"));
		_spineFeatureModel->appendRow(itm);
		itm->setData(static_cast<int>(common::SpineModules::Scores), Qt::UserRole);
	}
	if (spineFeatures & common::SpineModules::Multiplayer) {
		auto * itm = new QStandardItem(QApplication::tr("MultiplayerModule"));
		_spineFeatureModel->appendRow(itm);
		itm->setData(static_cast<int>(common::SpineModules::Multiplayer), Qt::UserRole);
	}
	if (spineFeatures & common::SpineModules::OverallSave) {
		auto * itm = new QStandardItem(QApplication::tr("OverallSaveModule"));
		_spineFeatureModel->appendRow(itm);
		itm->setData(static_cast<int>(common::SpineModules::OverallSave), Qt::UserRole);
	}
	if (spineFeatures & common::SpineModules::Gamepad) {
		auto * itm = new QStandardItem(QApplication::tr("GamepadModule"));
		_spineFeatureModel->appendRow(itm);
		itm->setData(static_cast<int>(common::SpineModules::Gamepad), Qt::UserRole);
	}
	if (spineFeatures & common::SpineModules::Friends) {
		auto * itm = new QStandardItem(QApplication::tr("FriendsModule"));
		_spineFeatureModel->appendRow(itm);
		itm->setData(static_cast<int>(common::SpineModules::Friends), Qt::UserRole);
	}
	if (spineFeatures & common::SpineModules::Statistics) {
		auto * itm = new QStandardItem(QApplication::tr("StatisticsModule"));
		_spineFeatureModel->appendRow(itm);
		itm->setData(static_cast<int>(common::SpineModules::Statistics), Qt::UserRole);
	}
	_spineFeaturesView->setVisible(_spineFeatureModel->rowCount());
	_moduleCheckBoxes[common::SpineModules::Achievements]->setChecked(spineFeatures & common::SpineModules::Achievements);
	_moduleCheckBoxes[common::SpineModules::Scores]->setChecked(spineFeatures & common::SpineModules::Scores);
	_moduleCheckBoxes[common::SpineModules::Multiplayer]->setChecked(spineFeatures & common::SpineModules::Multiplayer);
	_moduleCheckBoxes[common::SpineModules::OverallSave]->setChecked(spineFeatures & common::SpineModules::OverallSave);
	_moduleCheckBoxes[common::SpineModules::Gamepad]->setChecked(spineFeatures & common::SpineModules::Gamepad);
	_moduleCheckBoxes[common::SpineModules::Friends]->setChecked(spineFeatures & common::SpineModules::Friends);
	_moduleCheckBoxes[common::SpineModules::Statistics]->setChecked(spineFeatures & common::SpineModules::Statistics);

	_editInfoPageButton->setVisible(json.contains("EditRights"));

	_addImageButton->hide();
	_deleteImageButton->hide();
	_descriptionEdit->hide();
	_featuresEdit->hide();
	_spineFeaturesEdit->hide();
	_applyButton->hide();

	Database::DBError err;
	const bool installed = Database::queryCount(Config::BASEDIR.toStdString() + "/" + INSTALLED_DATABASE, "SELECT * FROM mods WHERE ModID = " + std::to_string(_projectID) + " LIMIT 1;", err) > 0;

	const auto installAllowed = json.contains("InstallAllowed");
	
	_installButton->setVisible(!installed && installAllowed);
	_installButton->setProperty("modid", static_cast<int>(_projectID));

	const QDirIterator it(Config::DOWNLOADDIR + "/mods/" + QString::number(_projectID) + "/System", QStringList() << "*.ini", QDir::Files, QDirIterator::Subdirectories);
	_startButton->setVisible(installed && it.hasNext() && !_runningUpdates.contains(_projectID));

	if (json.contains("Packages")) {
		for (auto jsonRef : json["Packages"].toArray()) {
			const auto jsonPackage = jsonRef.toObject();
			const auto packageName = decodeString(jsonPackage["Name"].toString());
			const auto packageID = jsonPackage["ID"].toString().toInt();
			const auto language = decodeString(jsonPackage["Language"].toString());
			
			const bool packageInstalled = Database::queryCount(Config::BASEDIR.toStdString() + "/" + INSTALLED_DATABASE, "SELECT PackageID FROM packages WHERE ModID = " + std::to_string(_projectID) + " AND PackageID = " + std::to_string(packageID) + " LIMIT 1;", err) > 0;
			auto * pb = new QPushButton(IconCache::getInstance()->getOrLoadIcon(":/svg/download.svg"), packageName, this);
			pb->setVisible(!packageInstalled && installed && installAllowed);
			pb->setProperty("packageid", packageID);
			pb->setProperty("language", language);
			_optionalPackageButtonsLayout->addWidget(pb, 0, Qt::AlignLeft);
			_optionalPackageButtons.append(pb);
			connect(pb, &QPushButton::released, this, &ModInfoPage::installPackage);
		}
	}

	const auto gameType = static_cast<common::GameType>(json["GameType"].toString().toInt());

	if (gameType == common::GameType::Game || _projectID == 36 || _projectID == 37 || _projectID == 116) {
		_startButton->setText(QApplication::tr("StartGame"));
		UPDATELANGUAGESETTEXT(_startButton, "StartGame");
	} else {
		_startButton->setText(QApplication::tr("StartMod"));
		UPDATELANGUAGESETTEXT(_startButton, "StartMod");
	}

	const auto hasHistory = json.contains("History");
	
	_historyBox->setVisible(hasHistory);

	if (hasHistory) {
		for (auto jsonRef : json["History"].toArray()) {
			auto * vl = new QVBoxLayout();
			auto * tb = new QTextBrowser(this);
			tb->setProperty("changelog", true);

			const auto jsonHistory = jsonRef.toObject();

			const auto changelog = decodeString(jsonHistory["Changelog"].toString()).trimmed();

			tb->setText(changelog.isEmpty() ? QApplication::tr("NoChangelogAvailable") : changelog);
			vl->addWidget(tb);

			const auto changelogMajorVersion = jsonHistory["MajorVersion"].toString().toInt();
			const auto changelogMinorVersion = jsonHistory["MinorVersion"].toString().toInt();
			const auto changelogPatchVersion = jsonHistory["PatchVersion"].toString().toInt();
			const auto changelogTimestamp = jsonHistory["Timestamp"].toString().toInt();
			const auto changelogSaveCompatible = jsonHistory["SaveCompatible"].toString().toInt();

			auto title = QString("%1.%2.%3 - %4").arg(changelogMajorVersion).arg(changelogMinorVersion).arg(changelogPatchVersion).arg(QDate(2000, 1, 1).addDays(changelogTimestamp).toString("dd.MM.yyyy"));

			if (!changelogSaveCompatible) {
				title += QString(" (%1)").arg(QApplication::tr("SaveNotCompatible"));
			}

			auto * s = new Spoiler(title, this);
			s->setContentLayout(vl);

			_historyLayout->addWidget(s);
			_historyWidgets.append(s);
		}
	}

	if (json.contains("Feedback")) {
		_feedbackButton->show();
	} else {
		_feedbackButton->hide();
	}

	if (_forceEdit) {
		switchToEdit();
	}
}

void ModInfoPage::installProject() {
	const int32_t modID = sender()->property("modid").toInt();
	emit tryInstallMod(modID, -1, InstallMode::UI);
}

void ModInfoPage::installPackage() {
	const int32_t packageID = sender()->property("packageid").toInt();
	emit tryInstallPackage(_projectID, packageID, InstallMode::UI);
}

void ModInfoPage::changePreviewImage(const QModelIndex & idx) {
	QString filename = _screens[idx.row()].first;
	if (!_screens[idx.row()].second.isEmpty()) { // downloaded screens: relative path
		filename.chop(2);
		const QPixmap preview(Config::DOWNLOADDIR + "/screens/" + QString::number(_projectID) + "/" + filename);
		_previewImageLabel->setPixmap(preview.scaled(QSize(640, 480), Qt::KeepAspectRatio, Qt::SmoothTransformation));
	} else {
		const QPixmap preview(filename);
		_previewImageLabel->setPixmap(preview.scaled(QSize(640, 480), Qt::KeepAspectRatio, Qt::SmoothTransformation));
	}
}

void ModInfoPage::switchToEdit() {
	_addImageButton->show();
	_deleteImageButton->show();
	_descriptionView->hide();
	_descriptionEdit->show();
	_spineFeaturesView->hide();
	_spineFeaturesEdit->show();
	_featuresEdit->show();
	_installButton->hide();
	_editInfoPageButton->hide();
	_ratingWidget->setVisible(false);
	_rateWidget->setVisible(false);
	_applyButton->show();
	_previewImageLabel->show();
	_thumbnailView->show();
	_addImageButton->setEnabled(_screens.size() < 10 || Config::Username == "Bonne");
	_historyBox->hide();
	_ratingsBox->hide();

	_forceEdit = false;
}

void ModInfoPage::forceEditPage() {
	_forceEdit = true;
}

void ModInfoPage::updateStarted(int projectID) {
	_runningUpdates.append(projectID);
}

void ModInfoPage::updateFinished(int projectID) {
	_runningUpdates.removeAll(projectID);
}

void ModInfoPage::editReview(int projectID, const QString & review) {
	if (_projectID != projectID) {
		loadPage(projectID);
	}

	_oldReview = review;
	_ownReviewEdit->setText(review);
	_ownReviewWidget->setVisible(true);
	_scrollArea->ensureWidgetVisible(_ownReviewWidget);
}

void ModInfoPage::addImage() {
	if (_screens.size() == 10 && Config::Username != "Bonne") return;

	const QString folder = Config::IniParser->value("PATH/Images", ".").toString();
	const QStringList paths = QFileDialog::getOpenFileNames(this, QApplication::tr("SelectImage"), folder, "Images (*.png *.jpg)");
	
	if (paths.isEmpty()) return;

	for (const auto & path : paths) {
		Config::IniParser->setValue("PATH/Images", QFileInfo(path).absolutePath());
		auto * itm = new QStandardItem();
		const QPixmap thumb(path);
		itm->setIcon(thumb.scaled(QSize(300, 100), Qt::KeepAspectRatio, Qt::SmoothTransformation));
		_thumbnailModel->appendRow(itm);
		_screens << qMakePair(path, QString());
		_addImageButton->setEnabled(_screens.size() < 10 || Config::Username == "Bonne");
	}
}

void ModInfoPage::removeImage() {
	if (!_thumbnailView->currentIndex().isValid()) return;

	const int row = _thumbnailView->currentIndex().row();
	_thumbnailModel->removeRow(row);
	_screens.erase(_screens.begin() + row);
}

void ModInfoPage::submitChanges() {
	WaitSpinner spinner(QApplication::tr("SubmittingPage"), this);

	QFutureWatcher<void> watcher(this);
	QEventLoop loop;
	connect(&watcher, &QFutureWatcher<void>::finished, &loop, &QEventLoop::quit);
	int projectID = _projectID;
	const auto description = encodeString(_descriptionEdit->toPlainText());
	QList<QString> features;
	for (QString s : _featuresEdit->text().split(";", Qt::SkipEmptyParts)) {
		features << encodeString(s);
	}
	int modules = 0;
	if (_moduleCheckBoxes[common::SpineModules::Achievements]->isChecked()) {
		modules |= common::SpineModules::Achievements;
	}
	if (_moduleCheckBoxes[common::SpineModules::Scores]->isChecked()) {
		modules |= common::SpineModules::Scores;
	}
	if (_moduleCheckBoxes[common::SpineModules::Multiplayer]->isChecked()) {
		modules |= common::SpineModules::Multiplayer;
	}
	if (_moduleCheckBoxes[common::SpineModules::OverallSave]->isChecked()) {
		modules |= common::SpineModules::OverallSave;
	}
	if (_moduleCheckBoxes[common::SpineModules::Gamepad]->isChecked()) {
		modules |= common::SpineModules::Gamepad;
	}
	if (_moduleCheckBoxes[common::SpineModules::Friends]->isChecked()) {
		modules |= common::SpineModules::Friends;
	}
	if (_moduleCheckBoxes[common::SpineModules::Statistics]->isChecked()) {
		modules |= common::SpineModules::Statistics;
	}
	const QFuture<void> future = QtConcurrent::run([this, projectID, description, features, modules]() {
		QJsonObject json;
		json["Language"] = Config::Language;
		json["ProjectID"] = projectID;
		json["Description"] = description;

		QJsonArray featuresArray;
		for (const auto & f : features) {
			featuresArray << f;
		}
		
		json["Features"] = featuresArray;
		json["SpineFeatures"] = modules;

		common::UploadScreenshotsMessage usm;
		usm.projectID = projectID;
		usm.username = q2s(Config::Username);
		usm.password = q2s(Config::Password);

		if (!_screens.isEmpty()) {
			QJsonArray screens;
			
			for (auto p : _screens) {
				if (p.second.isEmpty()) {
					// new screenshot
					// 1. calculate hash
					QString hashSum;
					const bool b = Hashing::hash(p.first, hashSum);
					if (b) {
						p.second = hashSum;
					}
					
					// 2. compress
					Compression::compress(p.first, false);
					p.first += ".z";
					// 3. add image to message
					
					const auto file = QFileInfo(p.first).fileName();
					{
						QFile f(p.first);
						if (f.open(QIODevice::ReadOnly)) {
							QByteArray byteArr = f.readAll();
							std::vector<uint8_t> buffer(byteArr.length());
							memcpy(&buffer[0], byteArr.data(), byteArr.length());

							usm.screenshots.emplace_back(q2s(file), buffer);
						}
						f.close();
						f.remove();
					}
					
					QJsonObject jsonScreen;
					jsonScreen["File"] = file;
					jsonScreen["Hash"] = p.second;

					screens << jsonScreen;
				} else {
					QJsonObject jsonScreen;
					jsonScreen["File"] = p.first;
					jsonScreen["Hash"] = p.second;

					screens << jsonScreen;
				}
			}

			json["Screenshots"] = screens;

			if (!usm.screenshots.empty()) {
				const auto serialized = usm.SerializeBlank();
				clockUtils::sockets::TcpSocket sock;
				if (sock.connectToHostname("clockwork-origins.com", SERVER_PORT, 10000) == clockUtils::ClockError::SUCCESS) {
					sock.writePacket(serialized);
				}
			}
		}

		Https::post(DATABASESERVER_PORT, "submitInfoPage", QJsonDocument(json).toJson(QJsonDocument::Compact), [this](const QJsonObject &, int) {});
	});
	watcher.setFuture(future);
	loop.exec();
	loadPage(_projectID);
}

void ModInfoPage::requestRandomMod() {
	QJsonObject json;
	json["Username"] = Config::Username;
	json["Password"] = Config::Password;
	json["Language"] = Config::Language;

	Https::postAsync(DATABASESERVER_PORT, "requestRandomPage", QJsonDocument(json).toJson(QJsonDocument::Compact), [this](const QJsonObject & responseData, int responseCode) {
		if (responseCode != 200) return;

		const int id = responseData["ID"].toString("-1").toInt();

		if (id == -1) return;

		emit gotRandomMod(id);
	});
}

void ModInfoPage::startProject() {
	QDirIterator it(Config::DOWNLOADDIR + "/mods/" + QString::number(_projectID) + "/System", QStringList() << "*.ini", QDir::Files, QDirIterator::Subdirectories);
	if (it.hasNext()) {
		it.next();
		emit triggerModStart(_projectID, it.filePath());
	}
}

void ModInfoPage::showFullscreen() {
	const auto * selectionModel = _thumbnailView->selectionModel();
	const QModelIndexList list = selectionModel->selectedRows();
	const int row = list.isEmpty() ? 0 : selectionModel->selectedRows().front().row();
	FullscreenPreview fp(Config::DOWNLOADDIR + "/screens/" + QString::number(_projectID) + "/" + _screens[row].first, this);
	fp.exec();
}

void ModInfoPage::selectedSpineFeature(const QModelIndex & idx) {
	if (idx.data(Qt::UserRole).toInt() == common::SpineModules::Achievements) {
		emit openAchievementView(_projectID, _projectNameLabel->text());
	} else if (idx.data(Qt::UserRole).toInt() == common::SpineModules::Scores) {
		emit openScoreView(_projectID, _projectNameLabel->text());
	}
}

void ModInfoPage::changedThumbnailSelection(QItemSelection selection) {
	if (!selection.empty()) {
		changePreviewImage(selection.indexes().first());
	}
}

void ModInfoPage::updateRatings(int rating1, int rating2, int rating3, int rating4, int rating5) {
	const int sum = rating1 + rating2 + rating3 + rating4 + rating5;

	_ratings[4].shareView->setMaximum(sum);
	_ratings[4].shareView->setValue(rating1);
	_ratings[4].text->setText(QString("(%1)").arg(i2s(rating1)));

	_ratings[3].shareView->setMaximum(sum);
	_ratings[3].shareView->setValue(rating2);
	_ratings[3].text->setText(QString("(%1)").arg(i2s(rating2)));

	_ratings[2].shareView->setMaximum(sum);
	_ratings[2].shareView->setValue(rating3);
	_ratings[2].text->setText(QString("(%1)").arg(i2s(rating3)));

	_ratings[1].shareView->setMaximum(sum);
	_ratings[1].shareView->setValue(rating4);
	_ratings[1].text->setText(QString("(%1)").arg(i2s(rating4)));

	_ratings[0].shareView->setMaximum(sum);
	_ratings[0].shareView->setValue(rating5);
	_ratings[0].text->setText(QString("(%1)").arg(i2s(rating5)));

	_ratingsBox->show();
}

void ModInfoPage::updateReviews(QJsonArray reviews) {
	for (auto jsonRef : reviews) {
		const auto json = jsonRef.toObject();

		const auto reviewer = json["Username"].toString();
		const auto review = json["Review"].toString();

		const auto rating = json["Rating"].toString().toInt();
		const auto duration = json["Duration"].toString().toInt();
		const auto reviewDuration = json["ReviewDuration"].toString().toInt();
		const auto date = json["Date"].toString().toInt();

		auto * rv = new ReviewWidget(reviewer, review, duration, reviewDuration, date, rating, _projectID, this);
		_reviewLayout->addWidget(rv);

		_reviewWidgets << rv;

		if (reviewer == Config::Username) {
			_ownReviewEdit->setText(review);
			_ownReviewWidget->setVisible(true);

			_oldReview = review;

			connect(this, &ModInfoPage::reviewChanged, rv, &ReviewWidget::setText);
		}
	}
}

void ModInfoPage::submitReview() {
	if (!_ownReviewWidget->isVisible()) return;

	const auto review = encodeString(_ownReviewEdit->toPlainText());

	if (review.isEmpty()) return;

	if (review == _oldReview) return;

	QJsonObject json;
	json["ProjectID"] = _projectID;
	json["Username"] = Config::Username;
	json["Password"] = Config::Password;
	json["Review"] = review;

	Https::postAsync(DATABASESERVER_PORT, "updateReview", QJsonDocument(json).toJson(QJsonDocument::Compact), [this](const QJsonObject &, int code) {
		if (code != 200) return;

		emit reviewSubmitted();
	});

	emit reviewChanged(review);
}

void ModInfoPage::feedbackClicked()
{
	Database::DBError err;
	const auto version = Database::queryNth<std::vector<int>, int, int, int>(Config::BASEDIR.toStdString() + "/" + UPDATES_DATABASE, "SELECT MajorVersion, MinorVersion, PatchVersion FROM updates WHERE ModID = " + std::to_string(_projectID) + " LIMIT 1;", err);

	const uint8_t versionMajor = version.empty() ? 0 : static_cast<uint8_t>(version[0]);
	const uint8_t versionMinor = version.empty() ? 0 : static_cast<uint8_t>(version[1]);
	const uint8_t versionPatch = version.empty() ? 0 : static_cast<uint8_t>(version[2]);

	FeedbackDialog dlg(_projectID, FeedbackDialog::Type::Project, versionMajor, versionMinor, versionPatch);
	dlg.exec();
}

void ModInfoPage::submittedReview()
{
	QMessageBox resultMsg(QMessageBox::Icon::NoIcon, QApplication::tr("ReviewSubmitted"), QApplication::tr("ReviewSubmittedText"), QMessageBox::StandardButton::Ok);
	resultMsg.button(QMessageBox::StandardButton::Ok)->setText(QApplication::tr("Ok"));
	resultMsg.setWindowFlags(resultMsg.windowFlags() & ~Qt::WindowContextHelpButtonHint);
	resultMsg.exec();
}

void ModInfoPage::mouseDoubleClickEvent(QMouseEvent * evt) {
	QWidget * child = childAt(evt->pos());
	if (child == _previewImageLabel) {
		showFullscreen();
	}
}

void ModInfoPage::showEvent(QShowEvent * evt) {
	QWidget::showEvent(evt);
	static bool firstOpen = true;
	if (firstOpen) {
		firstOpen = false;
		if (_projectID == -1) {
			requestRandomMod();
		}
	}
}

void ModInfoPage::showScreens() {
	int minWidth = std::numeric_limits<int>::max();
	int maxWidth = 0;
	
	QString filename = _screens[0].first;
	filename.chop(2);
	const QPixmap preview(Config::DOWNLOADDIR + "/screens/" + QString::number(_projectID) + "/" + filename);
	_previewImageLabel->setPixmap(preview.scaled(QSize(640, 480), Qt::KeepAspectRatio, Qt::SmoothTransformation));
	for (const auto & p : _screens) {
		QString fn = p.first;
		fn.chop(2); // .z
		auto * itm = new QStandardItem();
		QPixmap thumb(Config::DOWNLOADDIR + "/screens/" + QString::number(_projectID) + "/" + fn);
		QPixmap scaledThumb = thumb.scaled(QSize(300, 100), Qt::KeepAspectRatio, Qt::SmoothTransformation);
		itm->setIcon(scaledThumb);
		minWidth = std::min(minWidth, scaledThumb.width());
		maxWidth = std::max(maxWidth, scaledThumb.width());
		_thumbnailModel->appendRow(itm);
	}
	_previewImageLabel->setVisible(!_screens.empty());
	if (minWidth != std::numeric_limits<int>::max()) {
		_thumbnailView->setFixedWidth(std::min(width() - 50, static_cast<int>(_screens.size()) * (maxWidth + 15)));
	}
	_thumbnailView->setVisible(_screens.size() > 1);
}
