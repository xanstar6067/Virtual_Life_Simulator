#pragma once

namespace vlsui
{
enum class FeedingType { Unknown, Photosynthetic, Predator, Organic, Mixed };

// A description of observed feeding, not a permanent species or simulation rule.
// Match the feeding map's minimum evidence and require a two-thirds majority.
inline FeedingType ClassifyFeeding(int photosynthesis, int predation, int organics)
{
	const long long ps = photosynthesis > 0 ? photosynthesis : 0;
	const long long prey = predation > 0 ? predation : 0;
	const long long organic = organics > 0 ? organics : 0;
	const long long total = ps + prey + organic;
	if (total < 20) return FeedingType::Unknown;
	if (ps * 3 >= total * 2) return FeedingType::Photosynthetic;
	if (prey * 3 >= total * 2) return FeedingType::Predator;
	if (organic * 3 >= total * 2) return FeedingType::Organic;
	return FeedingType::Mixed;
}

inline const char* FeedingTypeName(FeedingType type)
{
	switch (type)
	{
	case FeedingType::Photosynthetic: return "Фотосинтетик";
	case FeedingType::Predator: return "Хищник";
	case FeedingType::Organic: return "Питается органикой";
	case FeedingType::Mixed: return "Смешанное питание";
	default: return "Питание пока не определено";
	}
}
}
