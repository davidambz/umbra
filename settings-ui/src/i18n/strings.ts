/**
 * The full shape of every user-facing string in settings-ui, per #95.
 * Each locale file in ./locales implements this interface — a locale
 * missing a key, or shaping one wrong (e.g. a string where a function is
 * expected), is a compile error, so translations can't silently drift out
 * of sync with what the English source actually needs.
 */
export interface Strings {
  common: {
    cancel: string;
    save: string;
    saving: string;
    delete: string;
  };
  wallpaperType: {
    video: string;
    image: string;
    web: string;
  };
  app: {
    loading: string;
    loadError: string;
    tablistAriaLabel: string;
    tabWallpapers: string;
    tabSettings: string;
  };
  monitorGrid: {
    empty: string;
    displayLabel: (displayIndex: number) => string;
  };
  monitorCard: {
    noWallpaper: string;
    unknownWallpaper: string;
    playlistSummary: (count: number) => string;
  };
  wallpaperLibrary: {
    heading: string;
    addButton: string;
    empty: string;
  };
  wallpaperCard: {
    quickAssignHint: string;
    quickAssignAriaLabel: (title: string) => string;
    deleteAriaLabel: (title: string) => string;
    deleteTitle: string;
    confirmDeleteTitle: (title: string) => string;
    confirmDeleteBody: string;
    confirmDeleteAssignedWarning: (displays: string) => string;
    disconnectedDisplay: string;
  };
  assignDialog: {
    monitorFieldLabel: string;
    primaryTag: string;
    modeFieldAriaLabel: string;
    modeNone: string;
    modeSingle: string;
    modePlaylist: string;
    emptyLibraryHint: string;
    wallpaperFieldLabel: string;
    fpsCapLabel: string;
    saveError: string;
  };
  wallpaperSelect: {
    placeholder: string;
  };
  playlistEditor: {
    rotationOrder: string;
    rotationEmptyHint: string;
    moveEarlier: string;
    moveLater: string;
    removeFromRotation: (title: string) => string;
    availableWallpapers: string;
    changeEvery: string;
    minutes: string;
    order: string;
    sequential: string;
    shuffle: string;
    unknownWallpaper: string;
  };
  addWallpaperDialog: {
    title: string;
    chooseFileAndImport: string;
    importing: string;
    titleFieldLabel: string;
    titlePlaceholder: string;
    typeFieldLabel: string;
    typeHintVideo: string;
    typeHintImage: string;
    typeHintWeb: string;
    note: string;
    cancelledMessage: string;
    genericFailedMessage: string;
  };
  settingsPanel: {
    appearanceHeading: string;
    displayHeading: string;
    startupHeading: string;
    themeLabel: string;
    themeDescription: string;
    themeSystem: string;
    themeLight: string;
    themeDark: string;
    languageLabel: string;
    languageDescription: string;
    languageSystem: string;
    syncLockScreenLabel: string;
    syncLockScreenDescription: string;
    syncMonitorsLabel: string;
    syncMonitorsDescription: string;
    launchOnStartupLabel: string;
    launchOnStartupDescription: string;
    pauseOnFullscreenLabel: string;
    pauseOnFullscreenDescription: string;
    pauseOnBatteryLabel: string;
    pauseOnBatteryDescription: string;
  };
  dialog: {
    closeAriaLabel: string;
  };
  updates: {
    heading: string;
    currentVersion: (version: string) => string;
    checkButton: string;
    checking: string;
    upToDate: string;
    updateAvailable: (version: string) => string;
    updateButton: string;
    installing: string;
    checkFailed: string;
  };
}
