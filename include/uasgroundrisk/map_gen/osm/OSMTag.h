#ifndef UGR_OSMTAG_H
#define UGR_OSMTAG_H
#include <string>

namespace ugr
{
	namespace mapping
	{
		namespace osm
		{
			struct OSMTag
			{
			public:
				std::string key, value;

				explicit OSMTag(std::string key) : key(key)
				{
				}

				OSMTag(std::string key, std::string value)
					: key(key), value(value)
				{
				}

				bool operator<(const OSMTag& other) const { return key < other.key; }

				std::string to_string() const
				{
					if (value.empty())
					{
						return key;
					}
					return key + "=" + value;
				}
			};
		}
	}
}
#endif // UGR_OSMTAG_H
