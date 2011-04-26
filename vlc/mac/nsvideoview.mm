#include "nsvideoview.h"

@implementation VideoView
- (void)addVoutSubview:(NSView *)view
{
      [view setFrame:[self bounds]];
      [self addSubview:view];
      [view setAutoresizingMask: NSViewHeightSizable |NSViewWidthSizable];
}
- (void)removeVoutSubview:(NSView *)view
{
      [view removeFromSuperview];
}
@end
