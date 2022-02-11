#ifndef UGR_OSMTAG_H
#define UGR_OSMTAG_H
#include <string>
#include <utility>

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

                explicit OSMTag(std::string key) : key(std::move(key))
                {
                }

                OSMTag(std::string key, std::string value)
                    : key(std::move(key)), value(std::move(value))
                {
                }

                friend bool operator<(const OSMTag& lhs, const OSMTag& rhs)
                {
                    return lhs.to_string() < rhs.to_string();
                }

                friend bool operator==(const OSMTag& lhs, const OSMTag& rhs)
                {
                    return lhs.to_string() == rhs.to_string();
                }

                friend bool operator!=(const OSMTag& lhs, const OSMTag& rhs)
                {
                    return !(lhs == rhs);
                }

                std::string to_string() const
                {
                    if (value.empty())
                    {
                        return key;
                    }
                    return key + "=" + value;
                }

                static OSMTag from_string(const std::string& str)
                {
                    const auto idx = str.find('=');
                    if (idx == std::string::npos)
                    {
                        return OSMTag(str);
                    }
                    const auto key = str.substr(0, idx);
                    const auto val = str.substr(idx + 1);
                    return {key, val};
                }
            };
        }
    }
}
#endif // UGR_OSMTAG_H
