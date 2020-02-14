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

#include "gamepad/GamepadSettingsWidget.h"

#include "gamepad/XBoxController.h"

#include "utils/Config.h"

#include "client/widgets/UpdateLanguage.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QVBoxLayout>

using namespace spine::gamepad;
using namespace spine::utils;

namespace {
	QString buttonToString(GamePadButton button) {
		switch (button) {
		case GamePadButton::GamePad_Button_DPAD_UP: {
			return "DPAD UP";
		}
		case GamePadButton::GamePad_Button_DPAD_DOWN: {
			return "DPAD DOWN";
		}
		case GamePadButton::GamePad_Button_DPAD_LEFT: {
			return "DPAD LEFT";
		}
		case GamePadButton::GamePad_Button_DPAD_RIGHT: {
			return "DPAD RIGHT";
		}
		case GamePadButton::GamePad_Button_START: {
			return "START";
		}
		case GamePadButton::GamePad_Button_BACK: {
			return "BACK";
		}
		case GamePadButton::GamePad_Button_LEFT_THUMB: {
			return "LEFT THUMB";
		}
		case GamePadButton::GamePad_Button_RIGHT_THUMB: {
			return "RIGHT THUMB";
		}
		case GamePadButton::GamePad_Button_LEFT_SHOULDER: {
			return "LB";
		}
		case GamePadButton::GamePad_Button_RIGHT_SHOULDER: {
			return "RB";
		}
		case GamePadButton::GamePad_Button_A: {
			return "A";
		}
		case GamePadButton::GamePad_Button_B: {
			return "B";
		}
		case GamePadButton::GamePad_Button_X: {
			return "X";
		}
		case GamePadButton::GamePad_Button_Y: {
			return "Y";
		}
		case GamePadButton::GamePad_LTrigger: {
			return "LT";
		}
		case GamePadButton::GamePad_RTrigger: {
			return "RT";
		}
		case GamePadButton::GamePad_LStick_X_Pos: {
			return "LSTICK RIGHT";
		}
		case GamePadButton::GamePad_LStick_X_Neg: {
			return "LSTICK LEFT";
		}
		case GamePadButton::GamePad_LStick_Y_Pos: {
			return "LSTICK UP";
		}
		case GamePadButton::GamePad_LStick_Y_Neg: {
			return "LSTICK DOWN";
		}
		case GamePadButton::GamePad_RStick_X_Pos: {
			return "RSTICK RIGHT";
		}
		case GamePadButton::GamePad_RStick_X_Neg: {
			return "RSTICK LEFT";
		}
		case GamePadButton::GamePad_RStick_Y_Pos: {
			return "RSTICK UP";
		}
		case GamePadButton::GamePad_RStick_Y_Neg: {
			return "RSTICK DOWN";
		}
		default: {
			return QApplication::tr("Unassigned");
		}
		}
	}
}

GamepadSettingsWidget::GamepadSettingsWidget(QWidget * par) : QWidget(par), _gamepadButtonToButtonMap(), _actionToButtonMap() {
	QVBoxLayout * l = new QVBoxLayout();
	l->setAlignment(Qt::AlignTop);

	_gamepadEnabled = new QCheckBox(QApplication::tr("GamepadActive"), this);
	UPDATELANGUAGESETTEXT(_gamepadEnabled, "GamepadActive");
	l->addWidget(_gamepadEnabled);

	{
		QHBoxLayout * hl = new QHBoxLayout();

		QLabel * lbl = new QLabel(QApplication::tr("UsedGamepad"), this);
		UPDATELANGUAGESETTEXT(lbl, "UsedGamepad");
		hl->addWidget(lbl);

		_controllerList = new QComboBox(this);
		for (int idx = GamePadIndex::GamePadIndex_One; idx < GamePadIndex::COUNT; idx++) {
			if (GamePadXbox::isConnected(GamePadIndex(idx))) {
				_controllerList->addItem(QApplication::tr("Controller").arg(idx + 1));
				_controllerList->setItemData(_controllerList->count() - 1, idx, Qt::UserRole);
			}
		}
		hl->addWidget(_controllerList);

		l->addLayout(hl);
	}
	{
		QHBoxLayout * hl = new QHBoxLayout();

		QLabel * lbl = new QLabel(QApplication::tr("KeyDelay"), this);
		UPDATELANGUAGESETTEXT(lbl, "KeyDelay");
		hl->addWidget(lbl);

		_keyDelayBox = new QSpinBox(this);
		_keyDelayBox->setMinimum(0);
		_keyDelayBox->setMaximum(1000);
		hl->addWidget(_keyDelayBox);

		l->addLayout(hl);
	}
	{
		QGroupBox * gb = new QGroupBox(QApplication::tr("AssignmentOfKeys"));
		UPDATELANGUAGESETTITLE(gb, "AssignmentOfKeys");
		QGridLayout * gl = new QGridLayout();

		int row = 0;
		{
			QLabel * lbl = new QLabel(QApplication::tr("MoveForward"), this);
			UPDATELANGUAGESETTEXT(lbl, "MoveForward");
			gl->addWidget(lbl, row % 14, 2 * (row / 14));

			QPushButton * pb = new QPushButton(QApplication::tr("Unassigned"), this);
			gl->addWidget(pb, row % 14, 2 * (row / 14) + 1);
			pb->setProperty("action", "keyUp");

			_actionToButtonMap.insert("keyUp", pb);

			connect(pb, SIGNAL(released()), this, SLOT(newButton()));

			row++;
		}
		{
			QLabel * lbl = new QLabel(QApplication::tr("MoveBackward"), this);
			UPDATELANGUAGESETTEXT(lbl, "MoveBackward");
			gl->addWidget(lbl, row % 14, 2 * (row / 14));

			QPushButton * pb = new QPushButton(QApplication::tr("Unassigned"), this);
			gl->addWidget(pb, row % 14, 2 * (row / 14) + 1);
			pb->setProperty("action", "keyDown");

			_actionToButtonMap.insert("keyDown", pb);

			connect(pb, SIGNAL(released()), this, SLOT(newButton()));

			row++;
		}
		{
			QLabel * lbl = new QLabel(QApplication::tr("TurnLeft"), this);
			UPDATELANGUAGESETTEXT(lbl, "TurnLeft");
			gl->addWidget(lbl, row % 14, 2 * (row / 14));

			QPushButton * pb = new QPushButton(QApplication::tr("Unassigned"), this);
			gl->addWidget(pb, row % 14, 2 * (row / 14) + 1);
			pb->setProperty("action", "keyLeft");

			_actionToButtonMap.insert("keyLeft", pb);

			connect(pb, SIGNAL(released()), this, SLOT(newButton()));

			row++;
		}
		{
			QLabel * lbl = new QLabel(QApplication::tr("TurnRight"), this);
			UPDATELANGUAGESETTEXT(lbl, "TurnRight");
			gl->addWidget(lbl, row % 14, 2 * (row / 14));

			QPushButton * pb = new QPushButton(QApplication::tr("Unassigned"), this);
			gl->addWidget(pb, row % 14, 2 * (row / 14) + 1);
			pb->setProperty("action", "keyRight");

			_actionToButtonMap.insert("keyRight", pb);

			connect(pb, SIGNAL(released()), this, SLOT(newButton()));

			row++;
		}
		{
			QLabel * lbl = new QLabel(QApplication::tr("ActionLeft"), this);
			UPDATELANGUAGESETTEXT(lbl, "ActionLeft");
			gl->addWidget(lbl, row % 14, 2 * (row / 14));

			QPushButton * pb = new QPushButton(QApplication::tr("Unassigned"), this);
			gl->addWidget(pb, row % 14, 2 * (row / 14) + 1);
			pb->setProperty("action", "keyActionLeft");

			_actionToButtonMap.insert("keyActionLeft", pb);

			connect(pb, SIGNAL(released()), this, SLOT(newButton()));

			row++;
		}
		{
			QLabel * lbl = new QLabel(QApplication::tr("ActionRight"), this);
			UPDATELANGUAGESETTEXT(lbl, "ActionRight");
			gl->addWidget(lbl, row % 14, 2 * (row / 14));

			QPushButton * pb = new QPushButton(QApplication::tr("Unassigned"), this);
			gl->addWidget(pb, row % 14, 2 * (row / 14) + 1);
			pb->setProperty("action", "keyActionRight");

			_actionToButtonMap.insert("keyActionRight", pb);

			connect(pb, SIGNAL(released()), this, SLOT(newButton()));

			row++;
		}
		{
			QLabel * lbl = new QLabel(QApplication::tr("Action"), this);
			UPDATELANGUAGESETTEXT(lbl, "Action");
			gl->addWidget(lbl, row % 14, 2 * (row / 14));

			QPushButton * pb = new QPushButton(QApplication::tr("Unassigned"), this);
			gl->addWidget(pb, row % 14, 2 * (row / 14) + 1);
			pb->setProperty("action", "keyAction");

			_actionToButtonMap.insert("keyAction", pb);

			connect(pb, SIGNAL(released()), this, SLOT(newButton()));

			row++;
		}
		{
			QLabel * lbl = new QLabel(QApplication::tr("StrafeLeft"), this);
			UPDATELANGUAGESETTEXT(lbl, "StrafeLeft");
			gl->addWidget(lbl, row % 14, 2 * (row / 14));

			QPushButton * pb = new QPushButton(QApplication::tr("Unassigned"), this);
			gl->addWidget(pb, row % 14, 2 * (row / 14) + 1);
			pb->setProperty("action", "keyStrafeLeft");

			_actionToButtonMap.insert("keyStrafeLeft", pb);

			connect(pb, SIGNAL(released()), this, SLOT(newButton()));

			row++;
		}
		{
			QLabel * lbl = new QLabel(QApplication::tr("StrafeRight"), this);
			UPDATELANGUAGESETTEXT(lbl, "StrafeRight");
			gl->addWidget(lbl, row % 14, 2 * (row / 14));

			QPushButton * pb = new QPushButton(QApplication::tr("Unassigned"), this);
			gl->addWidget(pb, row % 14, 2 * (row / 14) + 1);
			pb->setProperty("action", "keyStrafeRight");

			_actionToButtonMap.insert("keyStrafeRight", pb);

			connect(pb, SIGNAL(released()), this, SLOT(newButton()));

			row++;
		}
		{
			QLabel * lbl = new QLabel(QApplication::tr("Inventory"), this);
			UPDATELANGUAGESETTEXT(lbl, "Inventory");
			gl->addWidget(lbl, row % 14, 2 * (row / 14));

			QPushButton * pb = new QPushButton(QApplication::tr("Unassigned"), this);
			gl->addWidget(pb, row % 14, 2 * (row / 14) + 1);
			pb->setProperty("action", "keyInventory");

			_actionToButtonMap.insert("keyInventory", pb);

			connect(pb, SIGNAL(released()), this, SLOT(newButton()));

			row++;
		}
		{
			QLabel * lbl = new QLabel(QApplication::tr("Map"), this);
			UPDATELANGUAGESETTEXT(lbl, "Map");
			gl->addWidget(lbl, row % 14, 2 * (row / 14));

			QPushButton * pb = new QPushButton(QApplication::tr("Unassigned"), this);
			gl->addWidget(pb, row % 14, 2 * (row / 14) + 1);
			pb->setProperty("action", "keyShowMap");

			_actionToButtonMap.insert("keyShowMap", pb);

			connect(pb, SIGNAL(released()), this, SLOT(newButton()));

			row++;
		}
		{
			QLabel * lbl = new QLabel(QApplication::tr("Sneak"), this);
			UPDATELANGUAGESETTEXT(lbl, "Sneak");
			gl->addWidget(lbl, row % 14, 2 * (row / 14));

			QPushButton * pb = new QPushButton(QApplication::tr("Unassigned"), this);
			gl->addWidget(pb, row % 14, 2 * (row / 14) + 1);
			pb->setProperty("action", "keySneak");

			_actionToButtonMap.insert("keySneak", pb);

			connect(pb, SIGNAL(released()), this, SLOT(newButton()));

			row++;
		}
		{
			QLabel * lbl = new QLabel(QApplication::tr("Status"), this);
			UPDATELANGUAGESETTEXT(lbl, "Status");
			gl->addWidget(lbl, row % 14, 2 * (row / 14));

			QPushButton * pb = new QPushButton(QApplication::tr("Unassigned"), this);
			gl->addWidget(pb, row % 14, 2 * (row / 14) + 1);
			pb->setProperty("action", "keyShowStatus");

			_actionToButtonMap.insert("keyShowStatus", pb);

			connect(pb, SIGNAL(released()), this, SLOT(newButton()));

			row++;
		}
		{
			QLabel * lbl = new QLabel(QApplication::tr("Log"), this);
			UPDATELANGUAGESETTEXT(lbl, "Log");
			gl->addWidget(lbl, row % 14, 2 * (row / 14));

			QPushButton * pb = new QPushButton(QApplication::tr("Unassigned"), this);
			gl->addWidget(pb, row % 14, 2 * (row / 14) + 1);
			pb->setProperty("action", "keyShowLog");

			_actionToButtonMap.insert("keyShowLog", pb);

			connect(pb, SIGNAL(released()), this, SLOT(newButton()));

			row++;
		}
		{
			QLabel * lbl = new QLabel(QApplication::tr("Heal"), this);
			UPDATELANGUAGESETTEXT(lbl, "Heal");
			gl->addWidget(lbl, row % 14, 2 * (row / 14));

			QPushButton * pb = new QPushButton(QApplication::tr("Unassigned"), this);
			gl->addWidget(pb, row % 14, 2 * (row / 14) + 1);
			pb->setProperty("action", "keyHeal");

			_actionToButtonMap.insert("keyHeal", pb);

			connect(pb, SIGNAL(released()), this, SLOT(newButton()));

			row++;
		}
		{
			QLabel * lbl = new QLabel(QApplication::tr("Potion"), this);
			UPDATELANGUAGESETTEXT(lbl, "Potion");
			gl->addWidget(lbl, row % 14, 2 * (row / 14));

			QPushButton * pb = new QPushButton(QApplication::tr("Unassigned"), this);
			gl->addWidget(pb, row % 14, 2 * (row / 14) + 1);
			pb->setProperty("action", "keyPotion");

			_actionToButtonMap.insert("keyPotion", pb);

			connect(pb, SIGNAL(released()), this, SLOT(newButton()));

			row++;
		}
		{
			QLabel * lbl = new QLabel(QApplication::tr("LockTarget"), this);
			UPDATELANGUAGESETTEXT(lbl, "LockTarget");
			gl->addWidget(lbl, row % 14, 2 * (row / 14));

			QPushButton * pb = new QPushButton(QApplication::tr("Unassigned"), this);
			gl->addWidget(pb, row % 14, 2 * (row / 14) + 1);
			pb->setProperty("action", "keyLockTarget");

			_actionToButtonMap.insert("keyLockTarget", pb);

			connect(pb, SIGNAL(released()), this, SLOT(newButton()));

			row++;
		}
		{
			QLabel * lbl = new QLabel(QApplication::tr("Parade"), this);
			UPDATELANGUAGESETTEXT(lbl, "Parade");
			gl->addWidget(lbl, row % 14, 2 * (row / 14));

			QPushButton * pb = new QPushButton(QApplication::tr("Unassigned"), this);
			gl->addWidget(pb, row % 14, 2 * (row / 14) + 1);
			pb->setProperty("action", "keyParade");

			_actionToButtonMap.insert("keyParade", pb);

			connect(pb, SIGNAL(released()), this, SLOT(newButton()));

			row++;
		}
		{
			QLabel * lbl = new QLabel(QApplication::tr("Slow"), this);
			UPDATELANGUAGESETTEXT(lbl, "Slow");
			gl->addWidget(lbl, row % 14, 2 * (row / 14));

			QPushButton * pb = new QPushButton(QApplication::tr("Unassigned"), this);
			gl->addWidget(pb, row % 14, 2 * (row / 14) + 1);
			pb->setProperty("action", "keySlow");

			_actionToButtonMap.insert("keySlow", pb);

			connect(pb, SIGNAL(released()), this, SLOT(newButton()));

			row++;
		}
		{
			QLabel * lbl = new QLabel(QApplication::tr("Jump"), this);
			UPDATELANGUAGESETTEXT(lbl, "Jump");
			gl->addWidget(lbl, row % 14, 2 * (row / 14));

			QPushButton * pb = new QPushButton(QApplication::tr("Unassigned"), this);
			gl->addWidget(pb, row % 14, 2 * (row / 14) + 1);
			pb->setProperty("action", "keySMove");

			_actionToButtonMap.insert("keySMove", pb);

			connect(pb, SIGNAL(released()), this, SLOT(newButton()));

			row++;
		}
		{
			QLabel * lbl = new QLabel(QApplication::tr("DrawWeapon"), this);
			UPDATELANGUAGESETTEXT(lbl, "DrawWeapon");
			gl->addWidget(lbl, row % 14, 2 * (row / 14));

			QPushButton * pb = new QPushButton(QApplication::tr("Unassigned"), this);
			gl->addWidget(pb, row % 14, 2 * (row / 14) + 1);
			pb->setProperty("action", "keyWeapon");

			_actionToButtonMap.insert("keyWeapon", pb);

			connect(pb, SIGNAL(released()), this, SLOT(newButton()));

			row++;
		}
		{
			QLabel * lbl = new QLabel(QApplication::tr("DrawMeleeWeapon"), this);
			UPDATELANGUAGESETTEXT(lbl, "DrawMeleeWeapon");
			gl->addWidget(lbl, row % 14, 2 * (row / 14));

			QPushButton * pb = new QPushButton(QApplication::tr("Unassigned"), this);
			gl->addWidget(pb, row % 14, 2 * (row / 14) + 1);
			pb->setProperty("action", "keyDrawMeleeWeapon");

			_actionToButtonMap.insert("keyDrawMeleeWeapon", pb);

			connect(pb, SIGNAL(released()), this, SLOT(newButton()));

			row++;
		}
		{
			QLabel * lbl = new QLabel(QApplication::tr("DrawRangedWeapon"), this);
			UPDATELANGUAGESETTEXT(lbl, "DrawRangedWeapon");
			gl->addWidget(lbl, row % 14, 2 * (row / 14));

			QPushButton * pb = new QPushButton(QApplication::tr("Unassigned"), this);
			gl->addWidget(pb, row % 14, 2 * (row / 14) + 1);
			pb->setProperty("action", "keyDrawRangedWeapon");

			_actionToButtonMap.insert("keyDrawRangedWeapon", pb);

			connect(pb, SIGNAL(released()), this, SLOT(newButton()));

			row++;
		}
		{
			QLabel * lbl = new QLabel(QApplication::tr("Look"), this);
			UPDATELANGUAGESETTEXT(lbl, "Look");
			gl->addWidget(lbl, row % 14, 2 * (row / 14));

			QPushButton * pb = new QPushButton(QApplication::tr("Unassigned"), this);
			gl->addWidget(pb, row % 14, 2 * (row / 14) + 1);
			pb->setProperty("action", "keyLook");

			_actionToButtonMap.insert("keyLook", pb);

			connect(pb, SIGNAL(released()), this, SLOT(newButton()));

			row++;
		}
		{
			QLabel * lbl = new QLabel(QApplication::tr("LookFP"), this);
			UPDATELANGUAGESETTEXT(lbl, "LookFP");
			gl->addWidget(lbl, row % 14, 2 * (row / 14));

			QPushButton * pb = new QPushButton(QApplication::tr("Unassigned"), this);
			gl->addWidget(pb, row % 14, 2 * (row / 14) + 1);
			pb->setProperty("action", "keyLookFP");

			_actionToButtonMap.insert("keyLookFP", pb);

			connect(pb, SIGNAL(released()), this, SLOT(newButton()));

			row++;
		}
		{
			QLabel * lbl = new QLabel(QApplication::tr("PreviousSpell"), this);
			UPDATELANGUAGESETTEXT(lbl, "PreviousSpell");
			gl->addWidget(lbl, row % 14, 2 * (row / 14));

			QPushButton * pb = new QPushButton(QApplication::tr("Unassigned"), this);
			gl->addWidget(pb, row % 14, 2 * (row / 14) + 1);
			pb->setProperty("action", "keyPreviousSpell");

			_actionToButtonMap.insert("keyPreviousSpell", pb);

			connect(pb, SIGNAL(released()), this, SLOT(newButton()));

			row++;
		}
		{
			QLabel * lbl = new QLabel(QApplication::tr("NextSpell"), this);
			UPDATELANGUAGESETTEXT(lbl, "NextSpell");
			gl->addWidget(lbl, row % 14, 2 * (row / 14));

			QPushButton * pb = new QPushButton(QApplication::tr("Unassigned"), this);
			gl->addWidget(pb, row % 14, 2 * (row / 14) + 1);
			pb->setProperty("action", "keyNextSpell");

			_actionToButtonMap.insert("keyNextSpell", pb);

			connect(pb, SIGNAL(released()), this, SLOT(newButton()));

			row++;
		}
		{
			QLabel * lbl = new QLabel(QApplication::tr("DrawSpell"), this);
			UPDATELANGUAGESETTEXT(lbl, "DrawSpell");
			gl->addWidget(lbl, row % 14, 2 * (row / 14));

			QPushButton * pb = new QPushButton(QApplication::tr("Unassigned"), this);
			gl->addWidget(pb, row % 14, 2 * (row / 14) + 1);
			pb->setProperty("action", "keyDrawSpell");

			_actionToButtonMap.insert("keyDrawSpell", pb);

			connect(pb, SIGNAL(released()), this, SLOT(newButton()));

			row++;
		}
		{
			QLabel * lbl = new QLabel(QApplication::tr("Enter"), this);
			UPDATELANGUAGESETTEXT(lbl, "Enter");
			gl->addWidget(lbl, row % 14, 2 * (row / 14));

			QPushButton * pb = new QPushButton(QApplication::tr("Unassigned"), this);
			gl->addWidget(pb, row % 14, 2 * (row / 14) + 1);
			pb->setProperty("action", "keyEnter");

			_actionToButtonMap.insert("keyEnter", pb);

			connect(pb, SIGNAL(released()), this, SLOT(newButton()));
		}

		gb->setLayout(gl);
		l->addWidget(gb);
	}

	setLayout(l);

	connect(_gamepadEnabled, SIGNAL(stateChanged(int)), this, SLOT(changedGamepadState(int)));

	rejectSettings();

	changedGamepadState(_gamepadEnabled->checkState());
}

GamepadSettingsWidget::~GamepadSettingsWidget() {
}

void GamepadSettingsWidget::saveSettings() {
	Config::IniParser->beginGroup("GAMEPAD");
	Config::IniParser->setValue("enabled", _gamepadEnabled->isChecked());
	Config::IniParser->setValue("index", _controllerList->count() ? _controllerList->currentData(Qt::UserRole).toInt() : 0);
	Config::IniParser->setValue("keyDelay", _keyDelayBox->value());

	for (auto it = _gamepadButtonToButtonMap.begin(); it != _gamepadButtonToButtonMap.end(); ++it) {
		Config::IniParser->setValue(it.value()->property("action").toString(), it.key());
	}
	Config::IniParser->endGroup();
}

void GamepadSettingsWidget::rejectSettings() {
	Config::IniParser->beginGroup("GAMEPAD");
	const bool firstStartup = !Config::IniParser->contains("enabled");
	const bool enabled = Config::IniParser->value("enabled", false).toBool();
	_gamepadEnabled->setChecked(enabled);

	int i = Config::IniParser->value("index", 0).toInt();
	for (int j = 0; j < _controllerList->count(); j++) {
		if (_controllerList->itemData(j, Qt::UserRole).toInt() == i) {
			_controllerList->setCurrentIndex(j);
			break;
		}
	}

	i = Config::IniParser->value("keyDelay", 150).toInt();
	_keyDelayBox->setValue(i);

	if (firstStartup) {
		_gamepadButtonToButtonMap.insert(GamePadButton::GamePad_LStick_Y_Pos, _actionToButtonMap["keyUp"]);
		_gamepadButtonToButtonMap.insert(GamePadButton::GamePad_LStick_Y_Neg, _actionToButtonMap["keyDown"]);
		_gamepadButtonToButtonMap.insert(GamePadButton::GamePad_LStick_X_Pos, _actionToButtonMap["keyRight"]);
		_gamepadButtonToButtonMap.insert(GamePadButton::GamePad_LStick_X_Neg, _actionToButtonMap["keyLeft"]);
		_gamepadButtonToButtonMap.insert(GamePadButton::GamePad_Button_LEFT_THUMB, _actionToButtonMap["keySneak"]);
		_gamepadButtonToButtonMap.insert(GamePadButton::GamePad_Button_RIGHT_THUMB, _actionToButtonMap["keySlow"]);
		_gamepadButtonToButtonMap.insert(GamePadButton::GamePad_Button_START, _actionToButtonMap["keyShowStatus"]);
		_gamepadButtonToButtonMap.insert(GamePadButton::GamePad_Button_A, _actionToButtonMap["keyEnter"]);
		_gamepadButtonToButtonMap.insert(GamePadButton::GamePad_Button_X, _actionToButtonMap["keyDrawMeleeWeapon"]);
		_gamepadButtonToButtonMap.insert(GamePadButton::GamePad_Button_B, _actionToButtonMap["keyDrawRangedWeapon"]);
		_gamepadButtonToButtonMap.insert(GamePadButton::GamePad_Button_Y, _actionToButtonMap["keyShowLog"]);
		_gamepadButtonToButtonMap.insert(GamePadButton::GamePad_Button_LEFT_SHOULDER, _actionToButtonMap["keyInventory"]);
		_gamepadButtonToButtonMap.insert(GamePadButton::GamePad_Button_RIGHT_SHOULDER, _actionToButtonMap["keyWeapon"]);
		_gamepadButtonToButtonMap.insert(GamePadButton::GamePad_RTrigger, _actionToButtonMap["keyAction"]);
		_gamepadButtonToButtonMap.insert(GamePadButton::GamePad_LTrigger, _actionToButtonMap["keySMove"]);
		_gamepadButtonToButtonMap.insert(GamePadButton::GamePad_Button_DPAD_LEFT, _actionToButtonMap["keyPreviousSpell"]);
		_gamepadButtonToButtonMap.insert(GamePadButton::GamePad_Button_DPAD_RIGHT, _actionToButtonMap["keyNextSpell"]);
		_gamepadButtonToButtonMap.insert(GamePadButton::GamePad_Button_DPAD_UP, _actionToButtonMap["keyDrawSpell"]);
		_gamepadButtonToButtonMap.insert(GamePadButton::GamePad_Button_DPAD_DOWN, _actionToButtonMap["keyShowMap"]);
		for (auto it = _gamepadButtonToButtonMap.begin(); it != _gamepadButtonToButtonMap.end(); ++it) {
			const QString buttonText = buttonToString(it.key());
			it.value()->setText(buttonText);
		}
	} else {
		for (auto it = _actionToButtonMap.begin(); it != _actionToButtonMap.end(); ++it) {
			i = Config::IniParser->value(it.key(), GamePadButton_Max).toInt();
			it.value()->setText(buttonToString(GamePadButton(i)));
			if (i != GamePadButton_Max) {
				_gamepadButtonToButtonMap.insert(GamePadButton(i), it.value());
			}
		}
	}
	Config::IniParser->endGroup();
}

bool GamepadSettingsWidget::isEnabled() const {
	return _gamepadEnabled->isChecked() && _controllerList->count();
}

GamePadIndex GamepadSettingsWidget::getIndex() const {
	return _controllerList->count() ? GamePadIndex(_controllerList->currentData(Qt::UserRole).toInt()) : GamePadIndex::GamePadIndex_One;
}

int GamepadSettingsWidget::getKeyDelay() const {
	return _keyDelayBox->value();
}

QMap<QString, GamePadButton> GamepadSettingsWidget::getKeyMapping() const {
	QMap<QString, GamePadButton> map;
	for (auto it = _gamepadButtonToButtonMap.begin(); it != _gamepadButtonToButtonMap.end(); ++it) {
		for (auto it2 = _actionToButtonMap.begin(); it2 != _actionToButtonMap.end(); ++it2) {
			if (it.value() == it2.value()) {
				map.insert(it2.key(), it.key());
				break;
			}
		}
	}
	return map;
}

void GamepadSettingsWidget::changedGamepadState(int checkState) {
	_controllerList->setEnabled(checkState == Qt::CheckState::Checked);
	_keyDelayBox->setEnabled(checkState == Qt::CheckState::Checked);
}

void GamepadSettingsWidget::newButton() {
	QPushButton * pb = qobject_cast<QPushButton *>(sender());
	if (pb) {
		const GamePadButton button = GamePadXbox::getButtonPressed(GamePadIndex(_controllerList->count() ? _controllerList->currentData(Qt::UserRole).toInt() : 0));
		if (button == GamePadButton_Max) {
			return;
		}
		auto it = _gamepadButtonToButtonMap.find(button);
		if (it != _gamepadButtonToButtonMap.end()) { // button already used => remove assignment
			it.value()->setText(QApplication::tr("Unassigned"));
			_gamepadButtonToButtonMap.erase(it);
		}
		for (it = _gamepadButtonToButtonMap.begin(); it != _gamepadButtonToButtonMap.end(); ++it) {
			if (it.value() == pb) {
				_gamepadButtonToButtonMap.erase(it);
				break;
			}
		}
		pb->setText(buttonToString(button));
		_gamepadButtonToButtonMap.insert(button, pb);
	}
}
