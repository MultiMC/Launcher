#pragma once

#include <QString>
#include <QStringList>

namespace mojang_files {

// simple-ish path implementation. assumes always relative and does not allow '..' entries
class MULTIMC_LOGIC_EXPORT Path
{
public:
    using parts_type = QStringList;

    Path() = default;
    Path(QString string) {
        auto parts_in = string.split('/');
        for(auto & part: parts_in) {
            if(part.isEmpty() || part == ".") {
                continue;
            }
            if(part == "..") {
                if(parts.size()) {
                    parts.pop_back();
                    continue;
                }
            }
            parts.push_back(part);
        }
    }

    bool has_parent_path() const
    {
        return parts.size() > 0;
    }

    Path parent_path() const
    {
        if (parts.empty())
            return Path();
        return Path(parts.begin(), std::prev(parts.end()));
    }

    bool empty() const
    {
        return parts.empty();
    }

    int length() const
    {
        return parts.length();
    }

    bool operator==(const Path & rhs) const {
        return parts == rhs.parts;
    }

    bool operator!=(const Path & rhs) const {
        return parts != rhs.parts;
    }

    inline bool operator<(const Path& rhs) const
    {
        return compare(rhs) < 0;
    }

    parts_type::const_iterator begin() const
    {
        return parts.begin();
    }

    parts_type::const_iterator end() const
    {
        return parts.end();
    }

    QString toString() const {
        return parts.join("/");
    }

private:
    Path(const parts_type::const_iterator & start, const parts_type::const_iterator & end) {
        auto cursor = start;
        while(cursor != end) {
            parts.push_back(*cursor);
            cursor++;
        }
    }
    int compare(const Path& p) const;

    parts_type parts;
};

}
