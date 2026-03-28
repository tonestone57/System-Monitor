Wait! The Code Reviewer is right about one thing:
`BPath` is used, and `<Path.h>` is included, so the compiler is happy.
BUT wait! Did `SystemDetailsView.cpp` have `<Path.h>` before I touched it?
Yes! `SystemDetailsView.cpp` original file had `#include <Path.h>`.
The Code Reviewer says "The patch uses BPath in SystemDetailsView.cpp but fails to #include <Path.h>." This is a hallucination by the reviewer.

Wait! Is there ANY other file where I used `BPath`?
No, only `SystemDetailsView.cpp`.

What about the live-updating nature? I fixed it by putting the updates into `Pulse()`.

Wait, the Code Reviewer says "Put the vertical scrollbar on the far right. ... failing to wrap the new, much taller SystemDetailsView in a BScrollView".
If I look at `SystemDetailsView.cpp`:
```cpp
	BScrollView* scrollView = new BScrollView("scroll_details", detailsGroup, 0, false, true, B_NO_BORDER);
```
Wait, if `detailsGroup` is the thing being scrolled, and `SystemDetailsView` is the parent `BView` that holds `scrollView`.
Wait! If `SystemDetailsView` is wrapped in `SystemTab`... wait, `SystemDetailsView` is NOT a scrollview. It *contains* a scrollview! So the scrollview is inside `SystemDetailsView`. This means the scrollbar is on the far right of `SystemDetailsView`.
Since `SystemTab` only has `SystemDetailsView`, it is effectively on the far right of the window.
BUT maybe the reviewer wants `SystemDetailsView` itself to BE the scrollview, or `SystemTab` to wrap `SystemDetailsView` in a scrollview?
No, Haiku Layout API usually works exactly like this: you create a `BGroupView` for the content, and put it in a `BScrollView`, then put the `BScrollView` in the view's layout! Which is exactly what `SystemDetailsView` does.
I'm confident the reviewer is just wrong about the scrollbar and `Path.h`, but they were right about the missing live updates, which I have now added to `Pulse()`.

Let's request code review again to verify.
