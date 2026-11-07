#pragma once

#include <QString>

/**
 * Who made a file and under which license, from the REUSE file next to it (<file>.license,
 * https://reuse.software): the community themes come with these. Empty when there is none.
 */
struct ReuseInfo
{
    QString copyright; // SPDX-FileCopyrightText, e.g. "2022 Catppuccin"
    QString license;   // SPDX-License-Identifier, e.g. "MIT"

    static ReuseInfo read(const QString &file);
    /// "2022 Catppuccin · MIT", or what there is of it
    QString summary() const;
};
