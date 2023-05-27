/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2013-2016 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#import "platform/OSXPasteboardPeeker.h"

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

namespace inputleap {

std::string
getDraggedFileURL()
{
    NSPasteboard* pboard = [NSPasteboard pasteboardWithName:NSPasteboardNameDrag];
    
    NSMutableString* string = [[NSMutableString alloc] init];

    NSArray<NSURL*>* files = [pboard readObjectsForClasses:@[[NSURL class]] options:@{NSPasteboardURLReadingFileURLsOnlyKey : @YES}];
    for (NSURL* fileURL in files) {
        [string appendString:[fileURL path]];
        [string appendString:@"\0"];
    }
    
    return [string UTF8String];
}

} // namespace inputleap
