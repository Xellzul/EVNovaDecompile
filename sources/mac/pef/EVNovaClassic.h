/* EVNovaClassic.h -- the prefix of the 1.0.10 "EV Nova.ppc" fragment, InterfaceLib. Notes: Research/mac-1.0.10-pin.md section 7 */
#define TARGET_API_MAC_OS8 1
#define TARGET_API_MAC_CARBON 0
/* the accessors are calls here too: EVCarbonHack.o defines them, and it needs the fields */
#define ACCESSOR_CALLS_ARE_FUNCTIONS 1
/* Menus.h has these as Carbon's; Classic links the Monitor Tool's, which return an error */
#define ShowMenuBar ToolboxShowMenuBar
#define HideMenuBar ToolboxHideMenuBar
#include <EVNova1010.h>
#undef ShowMenuBar
#undef HideMenuBar
extern int32_t ShowMenuBar(void);
extern int32_t HideMenuBar(void);
