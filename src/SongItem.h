#pragma once
#include <QString>
#include <QRandomGenerator>
#include <cstdint>

class SongItem
{
public:
	QString caption;
	QString lyrics;
	uint64_t uniqueId;
	QString file;
	QString vocalLanguage;
	QString json;

	inline SongItem(const QString &caption = "", const QString &lyrics = "")
		: caption(caption), lyrics(lyrics)
	{
		// Generate a unique ID using a cryptographically secure random number
		uniqueId = QRandomGenerator::global()->generate64();
	}
};
