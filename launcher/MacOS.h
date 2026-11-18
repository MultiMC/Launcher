/* Copyright 2026 Petr Mrázek
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#pragma once

// Only for Objective-C++ (.mm) files.
//
// @available(macOS ...) needs __isPlatformVersionAtLeast from clang's compiler-rt, which the
// osxcross toolchain doesn't ship, so version checks are done by hand; the files using newer
// APIs behind them switch off clang's availability warnings.

#import <Foundation/Foundation.h>

inline bool macOSAtLeast(int major, int minor)
{
    NSOperatingSystemVersion version = {major, minor, 0};
    return [[NSProcessInfo processInfo] isOperatingSystemAtLeastVersion:version];
}
