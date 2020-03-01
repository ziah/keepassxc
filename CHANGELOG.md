# Changelog

## 2.5.3 (2020-01-19)

### Fixed

- Fix a possible database lockout when removing a YubiKey from a KDBX 3.1 database [#4147]
- Fix crash if Auto-Type is performed on a new entry [#4150]
- Fix crash when all entries are deleted from a group [#4156]
- Improve the reliability of clipboard clearing on Gnome [#4165]
- Do not check cmd:// URLs for valid URL syntax anymore [#4172]
- Prevent unnecessary merges for databases on network shares [#4153]
- Browser: Prevent native messaging proxy from blocking application shutdown [#4155]
- Browser: Improve website URL matching [#4134, #4177]

### Added

- Browser: Enable support for Chromium-based Edge Browser [#3359]

## 2.5.2 (2020-01-04)

### Added

- Browser: Show UI warning when entering invalid URLs [#3912]
- Browser: Option to use an entry only for HTTP auth [#3927]

### Changed

- Disable the user interface when merging or saving the database [#3991]
- Ability to hide protected attribute after reveal [#3877]
- Remove mention of "snaps" in Windows and macOS [#3879]
- CLI: Merge parameter for source database key file (--key-file-from) [#3961]
- Improve GUI tests reliability on Hi-DPI displays [#4075]
- Disable deprecation warnings to allow building with Qt 5.14+ [#4075]
- OPVault: Use 'otp' attribute for TOTP field imports [#4075]

### Fixed

- Fix crashes when saving a database to cloud storage [#3991]
- Fix crash when pressing enter twice while opening database [#3885]
- Fix handling of HTML when displayed in the entry preview panel [#3910]
- Fix start minimized to tray on Linux [#3899]
- Fix Auto Open with key file only databases [#4075]
- Fix escape key closing the standalone password generator [#3892]
- macOS: Fix monospace font usage in password field and notes [#4075]
- macOS: Fix building on macOS 10.9 to 10.11 [#3946]
- Fix TOTP setup dialog not closing on database lock [#4075]
- Browser: Fix condition where additional URLs are ignored [#4033]
- Browser: Fix subdomain matching to return only relevant site entries [#3854]
- Secret Service: Fix multiple crashes and incompatibilities [#3871, #4009, #4074]
- Secret Service: Fix searching of entries [#4008, #4036]
- Secret Service: Fix behavior when exposed group is recycled [#3914]
- CLI: Release the database instance before exiting interactive mode [#3889]
- Fix (most) memory leaks in tests [#3922]

## 2.5.1 (2019-11-11)

### Added

- Add programmatic use of the EntrySearcher [#3760]
- Explicitly clear database memory upon locking even if the object is not deleted immediately [#3824]
- macOS: Add ability to perform notarization of built package [#3827]

### Changed

- Reduce file hash checking to every 30 seconds to correct performance issues [#3724]
- Correct formatting of notes in entry preview widget [#3727]
- Improve performance and UX of database statistics page [#3780]
- Improve interface for key file selection to discourage use of the database file [#3807]
- Hide Auto-Type sequences column when not needed [#3794]
- macOS: Revert back to using Carbon API for hotkey detection [#3794]
- CLI: Do not show protected fields by default [#3710]

### Fixed

- Secret Service: Correct issues interfacing with various applications [#3761]
- Fix building without additional features [#3693]
- Fix handling TOTP secret keys that require padding [#3764]
- Fix database unlock dialog password field focus [#3764]
- Correctly label open databases as locked on launch [#3764]
- Prevent infinite recursion when two databases AutoOpen each other [#3764]
- Browser: Fix incorrect matching of invalid URLs [#3759]
- Properly stylize the application name on Linux [#3775]
- Show application icon on Plasma Wayland sessions [#3777]
- macOS: Check for Auto-Type permissions on use instead of at launch [#3794]

## 2.5.0 (2019-10-26)

### Added

- Add 'Paper Backup' aka 'Export to HTML file' to the 'Database' menu [#3277]
- Add statistics panel with information about the database (number of entries, number of unique passwords, etc.) to the Database Settings dialog [#2034]
- Add offline user manual accessible via the 'Help' menu [#3274]
- Add support for importing 1Password OpVault files [#2292]
- Implement Freedesktop.org secret storage DBus protocol so that KeePassXC can be used as a vault service by libsecret [#2726]
- Add support for OnlyKey as an alternative to YubiKeys (requires yubikey-personalization >= 1.20.0) [#3352]
- Add group sorting feature [#3282]
- Add feature to download favicons for all entries at once [#3169]
- Add word case option to passphrase generator [#3172]
- Add support for RFC6238-compliant TOTP hashes [#2972]
- Add UNIX man page for main program [#3665]
- Add 'Monospaced font' option to the notes field [#3321]
- Add support for key files in auto open [#3504]
- Add search field for filtering entries in Auto-Type dialog [#2955]
- Complete usernames based on known usernames from other entries [#3300]
- Parse hyperlinks in the notes field of the entry preview pane [#3596]
- Allow abbreviation of field names in entry search [#3440]
- Allow setting group icons recursively [#3273]
- Add copy context menu for username and password in Auto-Type dialog [#3038]
- Drop to background after copying a password to the clipboard [#3253]
- Add 'Lock databases' entry to tray icon menu [#2896]
- Add option to minimize window after unlocking [#3439]
- Add option to minimize window after opening a URL [#3302]
- Request accessibility permissions for Auto-Type on macOS [#3624]
- Browser: Add initial support for multiple URLs [#3558]
- Browser: Add entry-specific browser integration settings [#3444]
- CLI: Add offline HIBP checker (requires a downloaded HIBP dump) [#2707]
- CLI: Add 'flatten' option to the 'ls' command [#3276]
- CLI: Add password generation options to `Add` and `Edit` commands [#3275]
- CLI: Add XML import [#3572]
- CLI: Add CSV export to the 'export' command [#3278]
- CLI: Add `-y --yubikey` option for YubiKey [#3416]
- CLI: Add `--dry-run` option for merging databases [#3254]
- CLI: Add group commands (mv, mkdir and rmdir) [#3313].
- CLI: Add interactive shell mode command `open` [#3224]


### Changed

- Redesign database unlock dialog [ #3287]
- Rework the entry preview panel [ #3306]
- Move notes to General tab on Group Preview Panel [#3336]
- Enable entry actions when editing an entry and cleanup entry context menu  [#3641]
- Improve detection of external database changes  [#2389]
- Warn if user is trying to use a KDBX file as a key file [#3625]
- Add option to disable KeePassHTTP settings migrations prompt [#3349, #3344]
- Re-enabled Wayland support (no Auto-Type yet) [#3520, #3341]
- Add icon to 'Toggle Window' action in tray icon menu [#3244]
- Merge custom data between databases only when necessary [#3475]
- Improve various file-handling related issues when picking files using the system's file dialog [#3473]
- Add 'New Entry' context menu when no entries are selected [#3671]
- Reduce default Argon2 settings from 128 MiB and one thread per CPU core to 64 MiB and two threads to account for lower-spec mobile hardware [ #3672]
- Browser: Remove unused 'Remember' checkbox for HTTP Basic Auth [#3371]
- Browser: Show database name when pairing with a new browser [#3638]
- Browser: Show URL in allow access dialog [#3639]
- CLI: The password length option `-l` for the CLI commands `Add` and `Edit` is now `-L` [#3275]
- CLI: The `-u` shorthand for the `--upper` password generation option has been renamed to `-U` [#3275]
- CLI: Rename command `extract` to `export`. [#3277]

### Fixed

- Improve accessibility for assistive technologies [#3409]
- Correctly unlock all databases if `--pw-stdin` is provided [#2916]
- Fix password generator issues with special characters [#3303]
- Fix KeePassXC interrupting shutdown procedure [#3666]
- Fix password visibility toggle button state on unlock dialog [#3312]
- Fix potential data loss if database is reloaded while user is editing an entry [#3656]
- Fix hard-coded background color in search help popup [#3001]
- Fix font choice for password preview [#3425]
- Fix handling of read-only files when autosave is enabled [#3408]
- Handle symlinks correctly when atomic saves are disabled [#3463]
- Enable HighDPI icon scaling on Linux [#3332]
- Make Auto-Type on macOS more robust and remove old Carbon API calls [#3634, [#3347)]
- Hide Share tab if KeePassXC is compiled without KeeShare support and other minor KeeShare improvements [#3654, [#3291, #3029, #3031, #3236]
- Correctly bring window to the front when clicking tray icon on macOS [#3576]
- Correct application shortcut created by MSI Installer on Windows [#3296]
- Fix crash when removing custom data [#3508]
- Fix placeholder resolution in URLs [#3281]
- Fix various inconsistencies and platform-dependent compilation bugs [#3664, #3662, #3660, #3655, #3649, #3417, #3357, #3319, #3318, #3304]
- Browser: Fix potential leaking of entries through the browser integration API if multiple databases are opened [#3480]
- Browser: Fix password entropy calculation [#3107]
- Browser: Fix Windows registry settings for portable installation [#3603]

## 2.4.3 (2019-06-12)

### Added

- Add documentation for keyboard shortcuts to source code distribution [#3215]

### Fixed

- Fix library loading issues in the Snap and macOS releases [#3247]
- Fix various keyboard navigation issues [#3248]
- Fix main window toggling regression when clicking the tray icon on KDE [#3258]

## 2.4.2 (2019-05-31)

- Improve resilience against memory attacks - overwrite memory before free [#3020]
- Prevent infinite save loop when location is unavailable [#3026]
- Attempt to fix quitting application when shutdown or logout issued [#3199]
- Support merging database custom data [#3002]
- Fix opening URL's with non-http schemes [#3153]
- Fix data loss due to not reading all database attachments if duplicates exist [#3180]
- Fix entry context menu disabling when using keyboard navigation [#3199]
- Fix behaviors when canceling an entry edit [#3199]
- Fix processing of tray icon click and doubleclick [#3112]
- Update group in preview widget when focused [#3199]
- Prefer DuckDuckGo service over direct icon download (increases resolution) #2996]
- Remove apply button in application settings [#3019]
- Use winqtdeploy on Windows to correct deployment issues [#3025]
- Don't mark entry edit as modified when attribute selection changes [#3041]
- Use console code page CP_UTF8 on Windows if supported [#3050]
- Snap: Fix locking database with session lock [#3046]
- Snap: Fix theming across Linux distributions [#3057]
- Snap: Use SNAP_USER_COMMON and SNAP_USER_DATA directories [#3131]
- KeeShare: Automatically enable WITH_XC_KEESHARE_SECURE if quazip is found [#3088]
- macOS: Fix toolbar text when in dark mode [#2998]
- macOS: Lock database on switching user [#3097]
- macOS: Fix global Auto-Type when the database is locked[ #3138]
- Browser: Close popups when database is locked [#3093]
- Browser: Add tests [#3016]
- Browser: Don't create default group if custom group is enabled [#3127]

## 2.4.1 (2019-04-12)

- Fix database deletion when using unsafe saves to a different file system #2889
- Fix opening databases with legacy key files that contain '/' #2872
- Fix opening database files from the command line #2919
- Fix crash when editing master key #2836
- Fix multiple issues with apply button behavior #2947
- Fix issues on application startup (tab order, --pw-stdin, etc.) #2830
- Fix building without WITH_XC_KEESHARE
- Fix reference entry coloring on macOS dark mode #2984
- Hide window when performing entry auto-type on macOS #2969
- Improve UX of update checker; reduce checks to every 7 days #2968
- KeeShare improvements [#2946, #2978, #2824]
- Re-enable Ctrl+C to copy password from search box #2947
- Add KeePassXC-Browser integration for Brave browser #2933
- SSH Agent: Re-Add keys on database unlock #2982
- SSH Agent: Only remove keys on app exit if they are removed on lock #2985
- CLI: Add --no-password option #2708
- CLI: Improve database extraction to XML #2698
- CLI: Don't call mandb on build #2774
- CLI: Add debug info #2714
- Improve support for Snap theming #2832
- Add support for building on Haiku OS #2859
- Ctrl+PgDn now goes to the next tab and Ctrl+PgUp to the previous
- Fix compiling on GCC 5 / Xenial #2990
- Add .gitrev output to tarball for third-party builds #2970
- Add WITH_XC_UPDATECHECK compile flag to toggle the update checker #2968

## 2.4.0 (2019-03-19)

- New Database Wizard #1952
- Advanced Search #1797
- Automatic update checker #2648
- KeeShare database synchronization [#2109, #1992, #2738, #2742, #2746, #2739]
- Improve favicon fetching; transition to Duck-Duck-Go [#2795, #2011, #2439]
- Remove KeePassHttp support #1752
- CLI: output info to stderr for easier scripting #2558
- CLI: Add --quiet option #2507
- CLI: Add create command #2540
- CLI: Add recursive listing of entries #2345
- CLI: Fix stdin/stdout encoding on Windows #2425
- SSH Agent: Support OpenSSH for Windows #1994
- macOS: TouchID Quick Unlock #1851
- macOS: Multiple improvements; include CLI in DMG [#2165, #2331, #2583]
- Linux: Prevent Klipper from storing secrets in clipboard #1969
- Linux: Use polling based file watching for NFS #2171
- Linux: Enable use of browser plugin in Snap build #2802
- TOTP QR Code Generator #1167
- High-DPI Scaling for 4k screens #2404
- Make keyboard shortcuts more consistent #2431
- Warn user if deleting referenced entries #1744
- Allow toolbar to be hidden and repositioned [#1819, #2357]
- Increase max allowed database timeout to 12 hours #2173
- Password generator uses existing password length by default #2318
- Improve alert message box button labels #2376
- Show message when a database merge makes no changes #2551
- Browser Integration Enhancements [#1497, #2253, #1904, #2232, #1850, #2218, #2391, #2396, #2542, #2622, #2637, #2790]
- Overall Code Improvements [#2316, #2284, #2351, #2402, #2410, #2419, #2422, #2443, #2491, #2506, #2610, #2667, #2709, #2731]

## 2.3.4 (2018-08-21)

- Show all URL schemes in entry view #1768
- Disable merge when database is locked #1975
- Fix intermittent crashes with favorite icon downloads #1980
- Provide potential crash warning to Qt 5.5.x users #2211
- Disable apply button when creating new entry/group to prevent data loss #2204
- Allow for 12 hour timeout to lock idle database #2173
- Multiple SSH Agent fixes [#1981, #2117]
- Multiple Browser Integration enhancements [#1993, #2003, #2055, #2116, #2159, #2174, #2185]
- Fix browser proxy application not closing properly #2142
- Add real names and Patreon supporters to about dialog #2214
- Add settings button to toolbar, Donate button, and Report a Bug button to help menu #2214
- Enhancements to release-tool to appsign intermediate build products #2101


## 2.3.3 (2018-05-09)

- Fix crash when browser integration is enabled #1923

## 2.3.2 (2018-05-07)

- Enable high entropy ASLR on Windows #1747
- Enhance favicon fetching #1786
- Fix crash on Windows due to autotype #1691
- Fix dark tray icon changing all icons #1680
- Fix --pw-stdin not using getPassword function #1686
- Fix placeholders being resolved in notes #1907
- Enable auto-type start delay to be configurable #1908
- Browser: Fix native messaging reply size #1719
- Browser: Increase maximum buffer size #1720
- Browser: Enhance usability and functionality [#1810, #1822, #1830, #1884, #1906]
- SSH Agent: Parse aes-256-cbc/ctr keys #1682
- SSH Agent: Enhance usability and functionality [#1677, #1679, #1681, #1787]

## 2.3.1 (2018-03-06)

- Fix unnecessary automatic upgrade to KDBX 4.0 and prevent challenge-response key being stripped #1568
- Abort saving and show an error message when challenge-response fails #1659
- Support inner stream protection on all string attributes #1646
- Fix favicon downloads not finishing on some websites #1657
- Fix freeze due to invalid STDIN data #1628
- Correct issue with encrypted RSA SSH keys #1587
- Fix crash on macOS due to QTBUG-54832 #1607
- Show error message if ssh-agent communication fails #1614
- Fix --pw-stdin and filename parameters being ignored #1608
- Fix Auto-Type syntax check not allowing spaces and special characters #1626
- Fix reference placeholders in combination with Auto-Type #1649
- Fix qtbase translations not being loaded #1611
- Fix startup crash on Windows due to missing SVG libraries #1662
- Correct database tab order regression #1610
- Fix GCC 8 compilation error #1612
- Fix copying of advanced attributes on KDE #1640
- Fix member initialization of CategoryListWidgetDelegate #1613
- Fix inconsistent toolbar icon sizes and provide higher-quality icons #1616
- Improve preview panel geometry #1609

## 2.3.0 (2018-02-27)

- Add support for KDBX 4.0, Argon2 and ChaCha20 [#148, #1179, #1230, #1494]
- Add SSH Agent feature [#1098, #1450, #1463]
- Add preview panel with details of the selected entry [#879, #1338]
- Add more and configurable columns to entry table and allow copying of values by double click #1305
- Add KeePassXC-Browser API as a replacement for KeePassHTTP #608
- Deprecate KeePassHTTP #1392
- Add support for Steam one-time passwords #1206
- Add support for multiple Auto-Type sequences for a single entry #1390
- Adjust YubiKey HMAC-SHA1 challenge-response key generation for KDBX 4.0 #1060
- Replace qHttp with cURL for website icon downloads #1460
- Remove lock file #1231
- Add option to create backup file before saving #1385
- Ask to save a generated password before closing the entry password generator #1499
- Resolve placeholders recursively #1078
- Add Auto-Type button to the toolbar #1056
- Improve window focus handling for Auto-Type dialogs [#1204, #1490]
- Auto-Type dialog and password generator can now be exited with ESC [#1252, #1412]
- Add optional dark tray icon #1154
- Add new "Unsafe saving" option to work around saving problems with file sync services #1385
- Add IBus support to AppImage and additional image formats to Windows builds [#1534, #1537]
- Add diceware password generator to CLI #1406
- Add --key-file option to CLI [#816, #824]
- Add DBus interface for opening and closing KeePassXC databases #283
- Add KDBX compression options to database settings #1419
- Discourage use of old fixed-length key files in favor of arbitrary files [#1326, #1327]
- Correct reference resolution in entry fields #1486
- Fix window state and recent databases not being remembered on exit #1453
- Correct history item generation when configuring TOTP for an entry #1446
- Correct multiple TOTP bugs #1414
- Automatic saving after every change is now a default #279
- Allow creation of new entries during search #1398
- Correct menu issues on macOS #1335
- Allow compilation on OpenBSD #1328
- Improve entry attachments view [#1139, #1298]
- Fix auto lock for Gnome and Xfce [#910, #1249]
- Don't remember key files in file dialogs when the setting is disabled #1188
- Improve database merging and conflict resolution [#807, #1165]
- Fix macOS pasteboard issues #1202
- Improve startup times on some platforms #1205
- Hide the notes field by default #1124
- Toggle main window by clicking tray icon with the middle mouse button #992
- Fix custom icons not copied over when databases are merged #1008
- Allow use of DEL key to delete entries #914
- Correct intermittent crash due to stale history items #1527
- Sanitize newline characters in title, username and URL fields #1502
- Reopen previously opened databases in correct order #774
- Use system's zxcvbn library if available #701
- Implement various i18n improvements [#690, #875, #1436]

## 2.2.4 (2017-12-13)

- Prevent database corruption when locked #1219
- Fixes apply button not saving new entries #1141
- Switch to Consolas font on Windows for password edit #1229
- Multiple fixes to AppImage deployment [#1115, #1228]
- Fixes multiple memory leaks #1213
- Resize message close to 16x16 pixels #1253

## 2.2.2 (2017-10-22)

- Fixed entries with empty URLs being reported to KeePassHTTP clients #1031
- Fixed YubiKey detection and enabled CLI tool for AppImage binary #1100
- Added AppStream description #1082
- Improved TOTP compatibility and added new Base32 implementation #1069
- Fixed error handling when processing invalid cipher stream #1099
- Fixed double warning display when opening a database #1037
- Fixed unlocking databases with --pw-stdin #1087
- Added ability to override QT_PLUGIN_PATH environment variable for AppImages #1079
- Fixed transform seed not being regenerated when saving the database #1068
- Fixed only one YubiKey slot being polled #1048
- Corrected an issue with entry icons while merging #1008
- Corrected desktop and tray icons in Snap package #1030
- Fixed screen lock and Google fallback settings #1029

## 2.2.1 (2017-10-01)

- Corrected multiple snap issues [#934, #1011]
- Corrected multiple custom icon issues [#708, #719, #994]
- Corrected multiple Yubikey issues #880
- Fixed single instance preventing load on occasion #997
- Keep entry history when merging databases #970
- Prevent data loss if passwords were mismatched #1007
- Fixed crash after merge #941
- Added configurable auto-type default delay #703
- Unlock database dialog window comes to front #663
- Translation and compiling fixes

## 2.2.0 (2017-06-23)

- Added YubiKey 2FA integration for unlocking databases #127
- Added TOTP support #519
- Added CSV import tool [#146, #490]
- Added KeePassXC CLI tool #254
- Added diceware password generator #373
- Added support for entry references [#370, #378]
- Added support for Twofish encryption #167
- Enabled DEP and ASLR for in-memory protection #371
- Enabled single instance mode #510
- Enabled portable mode #645
- Enabled database lock on screensaver and session lock #545
- Redesigned welcome screen with common features and recent databases #292
- Multiple updates to search behavior [#168, #213, #374, #471, #603, #654]
- Added auto-type fields {CLEARFIELD}, {SPACE}, {{}, {}} [#267, #427, #480]
- Fixed auto-type errors on Linux #550
- Prompt user prior to executing a cmd:// URL #235
- Entry attributes can be protected (hidden) #220
- Added extended ascii to password generator #538
- Added new database icon to toolbar #289
- Added context menu entry to empty recycle bin in databases #520
- Added "apply" button to entry and group edit windows #624
- Added macOS tray icon and enabled minimize on close #583
- Fixed issues with unclean shutdowns [#170, #580]
- Changed keyboard shortcut to create new database to CTRL+SHIFT+N #515
- Compare window title to entry URLs #556
- Implemented inline error messages #162
- Ignore group expansion and other minor changes when making database "dirty" #464
- Updated license and copyright information on souce files #632
- Added contributors list to about dialog #629

## 2.1.4 (2017-04-09)

- Bumped KeePassHTTP version to 1.8.4.2
- KeePassHTTP confirmation window comes to foreground #466

## 2.1.3 (2017-03-03)

- Fix possible overflow in zxcvbn library #363
- Revert HiDPI setting to avoid problems on laptop screens #332
- Set file meta properties in Windows executable #330
- Suppress error message when auto-reloading a locked database #345
- Improve usability of question dialog when database is already locked by a different instance #346
- Fix compiler warnings in QHttp library #351
- Use unified toolbar on Mac OS X #361
- Fix an issue on X11 where the main window would be raised instead of closed on Alt+F4 #362

## 2.1.2 (2017-02-17)

- Ask for save location when creating a new database #302
- Remove Libmicrohttpd dependency to clean up the code and ensure better OS X compatibility [#317, #265]
- Prevent Qt from degrading Wifi network performance on certain platforms #318
- Visually refine user interface on OS X and other platforms #299
- Remove unusable tray icon setting on OS X #293
- Fix compositing glitches on Ubuntu and prevent flashing when minimizing to the tray at startup #307
- Fix AppImage tray icon on Ubuntu [#277, #273]
- Fix global menu disappearing after restoring KeePassXC from the tray on Ubuntu #276
- Fix result order in entry search #320
- Enable HiDPI scaling on supported platforms #315
- Remove empty directories from installation target #282

## 2.1.1 (2017-02-06)

- Enabled HTTP plugin build; plugin is disabled by default and limited to localhost #147
- Escape HTML in dialog boxes #247
- Corrected crashes in favicon download and password generator [#233, #226]
- Increase font size of password meter #228
- Fixed compatibility with Qt 5.8 #211
- Use consistent button heights in password generator #229

## 2.1.0 (2017-01-22)

- Show unlock dialog when using autotype on a closed database [#10, #89]
- Show different tray icon when database is locked [#37, #46]
- Support autotype on Windows and OS X [#42, #60, #63]
- Add delay feature to autotype [#76, #77]
- Add password strength meter [#84, #92]
- Add option for automatically locking the database when minimizing
  the window #57
- Add feature to download favicons and use them as entry icons #30
- Automatically reload and merge database when the file changed on
  disk [#22, #33, #93]
- Add tool for merging two databases [#22, #47, #143]
- Add --pw-stdin commandline option to unlock the database by providing
  a password on STDIN #54
- Add utility script for reading the database password from KWallet #55
- Fix some KeePassHTTP settings not being remembered [#34, #65]
- Make search box persistent [#15, #67, #157]
- Enhance search feature by scoping the search to selected group [#16, #118]
- Improve interaction between search field and entry list [#131, #141]
- Add stand-alone password-generator [#18, #92]
- Don't require password repetition when password is visible [#27, #92]
- Add support for entry attributes in autotype sequences #107
- Always focus password field when opening the database unlock widget [#116, #117]
- Fix compilation errors on various platforms [#53, #126, #130]
- Restructure and improve kdbx-extract utility #160

## 2.0.3 (2016-09-04)

- Improved error reporting when reading / writing databases fails. [#450, #462]
- Display an error message when opening a custom icon fails.
- Detect custom icon format based on contents instead of the filename. #512
- Keep symlink intact when saving databases. #442.
- Fix a crash when deleting parent group of recycle bin. #520
- Display a confirm dialog before moving an entry to the recycle bin. #447
- Repair UUIDs of inconsistent history items. #130
- Only include top-level windows in auto-type window list when using gnome-shell.
- Update translations.

## 2.0.2 (2016-02-02)

- Fix regression in database writer that caused it to strip certain special
  characters (characters from Unicode plane > 0).
- Fix bug in repair function that caused it to strip non-ASCII characters.

## 2.0.1 (2016-01-31)

- Flush temporary file before opening attachment. #390
- Disable password generator when showing entry in history mode. #422
- Strip invalid XML chars when writing databases. #392
- Add repair function to fix databases with invalid XML chars. #392
- Display custom icons scaled. #322
- Allow opening databases that have no password and keyfile. #391
- Fix crash when importing .kdb files with invalid icon ids. #425
- Update translations.

## 2.0 (2015-12-06)

- Improve UI of the search edit.
- Clear clipboard when locking databases. #342
- Enable Ctrl+M shortcut to minimize the window on all platforms. #329
- Show a better message when trying to open an old database format. #338
- Fix global auto-type behavior with some window managers.
- Show global auto-type window on the active desktop. #359
- Disable systray on OS X. #326
- Restore main window when clicking on the OS X docker icon. #326

## 2.0 Beta 2 (2015-09-06)

- Fix crash when locking with search UI open #309
- Fix file locking on Mac OS X #327
- Set default extension when saving a database [#79, #308]

## 2.0 Beta 1 (2015-07-18)

- Remember entry column sizes #159
- Add translations
- Support opening attachments directly
- Support cmd:// URLs #244
- Protect opened databases with a file lock #18
- Export to csv files #57
- Add optional tray icon #153
- Allow setting the default auto-type sequence for groups #175
- Make the kdbx parser more lenient
- Remove --password command line option #285

## 2.0 Alpha 6 (2014-04-12)

- Add option to lock databases after user inactivity #62
- Add compatibility with libgcrypt 1.6 #129
- Display passwords in monospace font #51
- Use an icon for the button that shows/masks passwords #38
- Add an option to show passwords by default #93
- Improve password generator design #122
- On Linux link .kdbx files with KeePassX
- Remember window size #154
- Disallow global auto-typing when the database is locked

## 2.0 Alpha 5 (2013-12-20)

- Support copying entries and groups using drag'n'drop #74
- Open last used databases on startup #36
- Made the kdbx file parser more robust
- Only edit entries on doubleclick (not single) or with enter key
- Allow removing multiple entries
- Added option to minimize window when copying data to clipboard
- Save password generator settings
- Fixed auto-type producing wrong chars in some keyboard configurations #116
- Added some more actions to the toolbar

## 2.0 Alpha 4 (2013-03-29)

- Add random password generator #52
- Merge the 'Description' tab into the 'Entry' tab #59
- Fix crash when deleting history items #56
- Fix crash on Mac OS X Mountain Lion during startup #50
- Improved KeePassX application icon #58

## 2.0 Alpha 3 (2012-10-27)

- Auto-Type on Linux / X11
- Database locking
- Fix database corruption when changing key transformation rounds #34
- Verify header data of kdbx files
- Add menu entry to open URLs in the browser
- Add menu entry to copy an entry attribute to clipboard

## 2.0 Alpha 2 (2012-07-02)

- Import kdb (KeePass 1) files #2
- Display history items #23
- Implement history item limits #16
- Group and entry icons can be set #22
- Add keyboard shortcuts
- Search in databases #24
- Sortable entry view
- Support building Mac OS X bundles

## 2.0 Alpha 1 (2012-05-07)

- First release.
