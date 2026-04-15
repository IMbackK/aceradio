/*
 * Copyright Carl Philipp Klemm 2026
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once
#include <QString>
#include <QRandomGenerator>
#include <cstdint>
#include <QJsonObject>
#include <memory>

class SongItem
{
public:
	QString caption;
	QString lyrics;
	unsigned int bpm;
	QString key;
	QString vocalLanguage;
	bool cotCaption;

	uint64_t uniqueId;
	QString file;
	QString json;
	std::shared_ptr<QByteArray> audioData;

	SongItem(const QString &caption = "", const QString &lyrics = "");
	SongItem(const QJsonObject& json);

	void store(QJsonObject& json) const;
	void load(const QJsonObject& json);
};
