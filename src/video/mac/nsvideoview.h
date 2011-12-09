/*
    Copyright (C) 2010 Benoit Calvez <benoit@litchis.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef NSVIDEOVIEW_H
#define NSVIDEOVIEW_H

#import <Cocoa/Cocoa.h>

@interface VideoView : NSView
- (void)addVoutSubview: (NSView *)view;
- (void)removeVoutSubview: (NSView *)view;
@end

#endif // NSVIDEOVIEW_H
