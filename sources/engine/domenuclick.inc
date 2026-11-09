/* body of DoMenuClick: the head is in mainscreen.c and, under MAC1010, in maininfo.c */
{
    short menu;
    short item;
#if defined(MAC1010) && !TARGET_API_MAC_CARBON
    Str255 name;
#endif

    menu = HiWord(menuChoice);
    item = LoWord(menuChoice);
    if (menu == kAppleMenu) {
        if (item == kAboutItem)
            DoAboutBox();
#if defined(MAC1010) && !TARGET_API_MAC_CARBON
        else {
            GetMenuItemText(GetMenuHandle(kAppleMenu), item, name);
            OpenDeskAcc(name);
        }
#endif
    } else if (menu == kFileMenu) {
        if (item == kQuitItem)
            gQuit = 1;
    } else if (menu == kEditMenu) {
        if (item == kPrefsItem && gameLoaded)
            DoPrefsDialog();
    }
    HiliteMenu(0);
}
