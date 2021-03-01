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

#pragma once

#include "ManagementCommon.h"

#include "widgets/management/IManagementWidget.h"

#include <QFutureWatcher>
#include <QWidget>

class QCheckBox;
class QGridLayout;
class QLineEdit;

namespace spine {
namespace gui {
	class WaitSpinner;
}
namespace client {
namespace widgets {

	class ScoresWidget : public QWidget, public IManagementWidget {
		Q_OBJECT

	public:
		ScoresWidget(QWidget * par);
		~ScoresWidget() override;

		void updateModList(QList<ManagementMod> modList);
		void selectedMod(int index);
		void updateView() override;

	signals:
		void removeSpinner();
		void loadedData(QList<ManagementScore>);

	private slots:
		void updateData(QList<ManagementScore> scores);
		void updateScores();
		void addScore();

	private:
		QList<ManagementMod> _mods;
		int _modIndex;
		QGridLayout * _layout;
		int _rowCount;
		QList<QWidget *> _widgets;
		QList<std::tuple<QLineEdit *, QLineEdit *, QLineEdit *, QLineEdit *>> _scoreEdits;
		QList<QCheckBox *> _scoreToggles;
		gui::WaitSpinner * _waitSpinner;

		QFutureWatcher<void> _futureWatcher;
	};

} /* namespace widgets */
} /* namespace client */
} /* namespace spine */
