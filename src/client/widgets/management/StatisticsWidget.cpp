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
    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */
// Copyright 2018 Clockwork Origins

#include "widgets/management/StatisticsWidget.h"

#include "utils/Conversion.h"

#include <QApplication>
#include <QGroupBox>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

namespace spine {
namespace widgets {

	StatisticsWidget::StatisticsWidget(QWidget * par) : QWidget(par), _mods(), _modIndex(-1) {
		QVBoxLayout * l = new QVBoxLayout();
		l->setAlignment(Qt::AlignTop);

		{
			QScrollArea * sa = new QScrollArea(this);
			QWidget * cw = new QWidget(sa);
			QVBoxLayout * vl = new QVBoxLayout();
			vl->setAlignment(Qt::AlignTop);
			cw->setLayout(vl);
			sa->setWidget(cw);
			sa->setWidgetResizable(true);
			sa->setProperty("default", true);
			cw->setProperty("default", true);

			l->addWidget(sa, 1);

			{
				QGroupBox * downloadsBox = new QGroupBox(QApplication::tr("Downloads"), cw);
				_downloadsLayout = new QVBoxLayout();
				downloadsBox->setLayout(_downloadsLayout);

				vl->addWidget(downloadsBox);
			}

			{
				QGroupBox * playersBox = new QGroupBox(QApplication::tr("Players"), cw);
				_playersLayout = new QVBoxLayout();
				playersBox->setLayout(_playersLayout);

				vl->addWidget(playersBox);
			}

			{
				QGroupBox * playtimesBox = new QGroupBox(QApplication::tr("Playtimes"), cw);
				_playtimesLayout = new QVBoxLayout();
				playtimesBox->setLayout(_playtimesLayout);

				vl->addWidget(playtimesBox);
			}

			{
				QGroupBox * achievementsBox = new QGroupBox(QApplication::tr("Achievements"), cw);
				_achievementsLayout = new QVBoxLayout();
				achievementsBox->setLayout(_achievementsLayout);

				vl->addWidget(achievementsBox);
			}
		}

		setLayout(l);
	}

	StatisticsWidget::~StatisticsWidget() {
	}

	void StatisticsWidget::updateModList(QList<client::ManagementMod> modList) {
		_mods = modList;
	}

	void StatisticsWidget::selectedMod(int index) {
		// TODO
		/*_modIndex = index;
		qDeleteAll(_labelList);
		_labelList.clear();
		{
			QLabel * overallDownloadsLabel = new QLabel(QApplication::tr("OverallDownloads"), this);
			QLabel * overallDownloadsCountLabel = new QLabel(QString::number(_mods[_modIndex].statistics.overallDownloads), this);
			QHBoxLayout * hl = new QHBoxLayout();
			hl->addWidget(overallDownloadsLabel, 0, Qt::AlignLeft);
			hl->addWidget(overallDownloadsCountLabel, 0, Qt::AlignRight);
			_downloadsLayout->addLayout(hl);

			_labelList.append(overallDownloadsLabel);
			_labelList.append(overallDownloadsCountLabel);
		}
		{
			auto m = _mods[_modIndex].statistics.downloadsPerVersion;
			for (auto it = m.cbegin(); it != m.cend(); ++it) {
				QLabel * downloadsLabel = new QLabel(QApplication::tr("DownloadsForVersion").arg(s2q(it->first)), this);
				QLabel * downloadsCountLabel = new QLabel(QString::number(it->second), this);
				QHBoxLayout * hl = new QHBoxLayout();
				hl->addWidget(downloadsLabel, 0, Qt::AlignLeft);
				hl->addWidget(downloadsCountLabel, 0, Qt::AlignRight);
				_downloadsLayout->addLayout(hl);

				_labelList.append(downloadsLabel);
				_labelList.append(downloadsCountLabel);
			}
		}
		{
			QLabel * playersLabel = new QLabel(QApplication::tr("PlayersOverall"), this);
			QLabel * playersCountLabel = new QLabel(QString::number(_mods[_modIndex].statistics.overallPlayerCount), this);
			QHBoxLayout * hl = new QHBoxLayout();
			hl->addWidget(playersLabel, 0, Qt::AlignLeft);
			hl->addWidget(playersCountLabel, 0, Qt::AlignRight);
			_playersLayout->addLayout(hl);

			_labelList.append(playersLabel);
			_labelList.append(playersCountLabel);
		}
		{
			QLabel * playersLabel = new QLabel(QApplication::tr("Players24Hours"), this);
			QLabel * playersCountLabel = new QLabel(QString::number(_mods[_modIndex].statistics.last24HoursPlayerCount), this);
			QHBoxLayout * hl = new QHBoxLayout();
			hl->addWidget(playersLabel, 0, Qt::AlignLeft);
			hl->addWidget(playersCountLabel, 0, Qt::AlignRight);
			_playersLayout->addLayout(hl);

			_labelList.append(playersLabel);
			_labelList.append(playersCountLabel);
		}
		{
			QLabel * playersLabel = new QLabel(QApplication::tr("Players7Days"), this);
			QLabel * playersCountLabel = new QLabel(QString::number(_mods[_modIndex].statistics.last7DaysPlayerCount), this);
			QHBoxLayout * hl = new QHBoxLayout();
			hl->addWidget(playersLabel, 0, Qt::AlignLeft);
			hl->addWidget(playersCountLabel, 0, Qt::AlignRight);
			_playersLayout->addLayout(hl);

			_labelList.append(playersLabel);
			_labelList.append(playersCountLabel);
		}
		{
			QLabel * minPlaytimeLabel = new QLabel(QApplication::tr("MinPlaytime"), this);
			QLabel * minPlaytimeCountLabel = new QLabel(timeToString(_mods[_modIndex].statistics.minPlaytime), this);
			QHBoxLayout * hl = new QHBoxLayout();
			hl->addWidget(minPlaytimeLabel, 0, Qt::AlignLeft);
			hl->addWidget(minPlaytimeCountLabel, 0, Qt::AlignRight);
			_playtimesLayout->addLayout(hl);

			_labelList.append(minPlaytimeLabel);
			_labelList.append(minPlaytimeCountLabel);
		}
		{
			QLabel * maxPlaytimeLabel = new QLabel(QApplication::tr("MaxPlaytime"), this);
			QLabel * maxPlaytimeCountLabel = new QLabel(timeToString(_mods[_modIndex].statistics.maxPlaytime), this);
			QHBoxLayout * hl = new QHBoxLayout();
			hl->addWidget(maxPlaytimeLabel, 0, Qt::AlignLeft);
			hl->addWidget(maxPlaytimeCountLabel, 0, Qt::AlignRight);
			_playtimesLayout->addLayout(hl);

			_labelList.append(maxPlaytimeLabel);
			_labelList.append(maxPlaytimeCountLabel);
		}
		{
			QLabel * medianPlaytimeLabel = new QLabel(QApplication::tr("MedianPlaytime"), this);
			QLabel * medianPlaytimeCountLabel = new QLabel(timeToString(_mods[_modIndex].statistics.medianPlaytime), this);
			QHBoxLayout * hl = new QHBoxLayout();
			hl->addWidget(medianPlaytimeLabel, 0, Qt::AlignLeft);
			hl->addWidget(medianPlaytimeCountLabel, 0, Qt::AlignRight);
			_playtimesLayout->addLayout(hl);

			_labelList.append(medianPlaytimeLabel);
			_labelList.append(medianPlaytimeCountLabel);
		}
		{
			QLabel * avgPlaytimeLabel = new QLabel(QApplication::tr("AvgPlaytime"), this);
			QLabel * avgPlaytimeCountLabel = new QLabel(timeToString(_mods[_modIndex].statistics.avgPlaytime), this);
			QHBoxLayout * hl = new QHBoxLayout();
			hl->addWidget(avgPlaytimeLabel, 0, Qt::AlignLeft);
			hl->addWidget(avgPlaytimeCountLabel, 0, Qt::AlignRight);
			_playtimesLayout->addLayout(hl);

			_labelList.append(avgPlaytimeLabel);
			_labelList.append(avgPlaytimeCountLabel);
		}
		{
			QLabel * minSessiontimeLabel = new QLabel(QApplication::tr("MinSessiontime"), this);
			QLabel * minSessiontimeCountLabel = new QLabel(timeToString(_mods[_modIndex].statistics.minSessiontime), this);
			QHBoxLayout * hl = new QHBoxLayout();
			hl->addWidget(minSessiontimeLabel, 0, Qt::AlignLeft);
			hl->addWidget(minSessiontimeCountLabel, 0, Qt::AlignRight);
			_playtimesLayout->addLayout(hl);

			_labelList.append(minSessiontimeLabel);
			_labelList.append(minSessiontimeCountLabel);
		}
		{
			QLabel * maxSessiontimeLabel = new QLabel(QApplication::tr("MaxSessiontime"), this);
			QLabel * maxSessiontimeCountLabel = new QLabel(timeToString(_mods[_modIndex].statistics.maxSessiontime), this);
			QHBoxLayout * hl = new QHBoxLayout();
			hl->addWidget(maxSessiontimeLabel, 0, Qt::AlignLeft);
			hl->addWidget(maxSessiontimeCountLabel, 0, Qt::AlignRight);
			_playtimesLayout->addLayout(hl);

			_labelList.append(maxSessiontimeLabel);
			_labelList.append(maxSessiontimeCountLabel);
		}
		{
			QLabel * medianSessiontimeLabel = new QLabel(QApplication::tr("MedianSessiontime"), this);
			QLabel * medianSessiontimeCountLabel = new QLabel(timeToString(_mods[_modIndex].statistics.medianSessiontime), this);
			QHBoxLayout * hl = new QHBoxLayout();
			hl->addWidget(medianSessiontimeLabel, 0, Qt::AlignLeft);
			hl->addWidget(medianSessiontimeCountLabel, 0, Qt::AlignRight);
			_playtimesLayout->addLayout(hl);

			_labelList.append(medianSessiontimeLabel);
			_labelList.append(medianSessiontimeCountLabel);
		}
		{
			QLabel * avgSessiontimeLabel = new QLabel(QApplication::tr("AvgSessiontime"), this);
			QLabel * avgSessiontimeCountLabel = new QLabel(timeToString(_mods[_modIndex].statistics.avgSessiontime), this);
			QHBoxLayout * hl = new QHBoxLayout();
			hl->addWidget(avgSessiontimeLabel, 0, Qt::AlignLeft);
			hl->addWidget(avgSessiontimeCountLabel, 0, Qt::AlignRight);
			_playtimesLayout->addLayout(hl);

			_labelList.append(avgSessiontimeLabel);
			_labelList.append(avgSessiontimeCountLabel);
		}
		{
			auto m = _mods[_modIndex].statistics.achievementStatistics;
			for (auto it = m.cbegin(); it != m.cend(); ++it) {
				{
					QLabel * achievementLabel = new QLabel(QApplication::tr("MinAchievementTime").arg(s2q(it->name)), this);
					QLabel * achievementCountLabel = new QLabel(timeToString(it->minTime), this);
					QHBoxLayout * hl = new QHBoxLayout();
					hl->addWidget(achievementLabel, 0, Qt::AlignLeft);
					hl->addWidget(achievementCountLabel, 0, Qt::AlignRight);
					_achievementsLayout->addLayout(hl);

					_labelList.append(achievementLabel);
					_labelList.append(achievementCountLabel);
				}
				{
					QLabel * achievementLabel = new QLabel(QApplication::tr("MaxAchievementTime").arg(s2q(it->name)), this);
					QLabel * achievementCountLabel = new QLabel(timeToString(it->maxTime), this);
					QHBoxLayout * hl = new QHBoxLayout();
					hl->addWidget(achievementLabel, 0, Qt::AlignLeft);
					hl->addWidget(achievementCountLabel, 0, Qt::AlignRight);
					_achievementsLayout->addLayout(hl);

					_labelList.append(achievementLabel);
					_labelList.append(achievementCountLabel);
				}
				{
					QLabel * achievementLabel = new QLabel(QApplication::tr("MedianAchievementTime").arg(s2q(it->name)), this);
					QLabel * achievementCountLabel = new QLabel(timeToString(it->medianTime), this);
					QHBoxLayout * hl = new QHBoxLayout();
					hl->addWidget(achievementLabel, 0, Qt::AlignLeft);
					hl->addWidget(achievementCountLabel, 0, Qt::AlignRight);
					_achievementsLayout->addLayout(hl);

					_labelList.append(achievementLabel);
					_labelList.append(achievementCountLabel);
				}
				{
					QLabel * achievementLabel = new QLabel(QApplication::tr("AvgAchievementTime").arg(s2q(it->name)), this);
					QLabel * achievementCountLabel = new QLabel(timeToString(it->avgTime), this);
					QHBoxLayout * hl = new QHBoxLayout();
					hl->addWidget(achievementLabel, 0, Qt::AlignLeft);
					hl->addWidget(achievementCountLabel, 0, Qt::AlignRight);
					_achievementsLayout->addLayout(hl);

					_labelList.append(achievementLabel);
					_labelList.append(achievementCountLabel);
				}
			}
		}*/
	}

} /* namespace widgets */
} /* namespace spine */
