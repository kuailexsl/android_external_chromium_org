// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/shell_integration_linux.h"

#include <fcntl.h>
#include <glib.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>
#include <vector>

#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/environment.h"
#include "base/file_util.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/i18n/file_util_icu.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/path_service.h"
#include "base/posix/eintr_wrapper.h"
#include "base/process/kill.h"
#include "base/process/launch.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_tokenizer.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread.h"
#include "base/threading/thread_restrictions.h"
#include "build/build_config.h"
#include "chrome/browser/web_applications/web_app.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_version_info.h"
#include "content/public/browser/browser_thread.h"
#include "ui/gfx/image/image_family.h"
#include "url/gurl.h"

using content::BrowserThread;

namespace {

// Helper to launch xdg scripts. We don't want them to ask any questions on the
// terminal etc. The function returns true if the utility launches and exits
// cleanly, in which case |exit_code| returns the utility's exit code.
bool LaunchXdgUtility(const std::vector<std::string>& argv, int* exit_code) {
  // xdg-settings internally runs xdg-mime, which uses mv to move newly-created
  // files on top of originals after making changes to them. In the event that
  // the original files are owned by another user (e.g. root, which can happen
  // if they are updated within sudo), mv will prompt the user to confirm if
  // standard input is a terminal (otherwise it just does it). So make sure it's
  // not, to avoid locking everything up waiting for mv.
  *exit_code = EXIT_FAILURE;
  int devnull = open("/dev/null", O_RDONLY);
  if (devnull < 0)
    return false;
  base::FileHandleMappingVector no_stdin;
  no_stdin.push_back(std::make_pair(devnull, STDIN_FILENO));

  base::ProcessHandle handle;
  base::LaunchOptions options;
  options.fds_to_remap = &no_stdin;
  if (!base::LaunchProcess(argv, options, &handle)) {
    close(devnull);
    return false;
  }
  close(devnull);

  return base::WaitForExitCode(handle, exit_code);
}

std::string CreateShortcutIcon(
    const ShellIntegration::ShortcutInfo& shortcut_info,
    const base::FilePath& shortcut_filename) {
  if (shortcut_info.favicon.empty())
    return std::string();

  // TODO(phajdan.jr): Report errors from this function, possibly as infobars.
  base::ScopedTempDir temp_dir;
  if (!temp_dir.CreateUniqueTempDir())
    return std::string();

  base::FilePath temp_file_path = temp_dir.path().Append(
      shortcut_filename.ReplaceExtension("png"));
  std::string icon_name = temp_file_path.BaseName().RemoveExtension().value();

  for (gfx::ImageFamily::const_iterator it = shortcut_info.favicon.begin();
       it != shortcut_info.favicon.end(); ++it) {
    int width = it->Width();
    scoped_refptr<base::RefCountedMemory> png_data = it->As1xPNGBytes();
    if (png_data->size() == 0) {
      // If the bitmap could not be encoded to PNG format, skip it.
      LOG(WARNING) << "Could not encode icon " << icon_name << ".png at size "
                   << width << ".";
      continue;
    }
    int bytes_written = file_util::WriteFile(temp_file_path,
        reinterpret_cast<const char*>(png_data->front()), png_data->size());

    if (bytes_written != static_cast<int>(png_data->size()))
      return std::string();

    std::vector<std::string> argv;
    argv.push_back("xdg-icon-resource");
    argv.push_back("install");

    // Always install in user mode, even if someone runs the browser as root
    // (people do that).
    argv.push_back("--mode");
    argv.push_back("user");

    argv.push_back("--size");
    argv.push_back(base::IntToString(width));

    argv.push_back(temp_file_path.value());
    argv.push_back(icon_name);
    int exit_code;
    if (!LaunchXdgUtility(argv, &exit_code) || exit_code) {
      LOG(WARNING) << "Could not install icon " << icon_name << ".png at size "
                   << width << ".";
    }
  }
  return icon_name;
}

bool CreateShortcutOnDesktop(const base::FilePath& shortcut_filename,
                             const std::string& contents) {
  // Make sure that we will later call openat in a secure way.
  DCHECK_EQ(shortcut_filename.BaseName().value(), shortcut_filename.value());

  base::FilePath desktop_path;
  if (!PathService::Get(base::DIR_USER_DESKTOP, &desktop_path))
    return false;

  int desktop_fd = open(desktop_path.value().c_str(), O_RDONLY | O_DIRECTORY);
  if (desktop_fd < 0)
    return false;

  int fd = openat(desktop_fd, shortcut_filename.value().c_str(),
                  O_CREAT | O_EXCL | O_WRONLY,
                  S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
  if (fd < 0) {
    if (IGNORE_EINTR(close(desktop_fd)) < 0)
      PLOG(ERROR) << "close";
    return false;
  }

  ssize_t bytes_written = file_util::WriteFileDescriptor(fd, contents.data(),
                                                         contents.length());
  if (IGNORE_EINTR(close(fd)) < 0)
    PLOG(ERROR) << "close";

  if (bytes_written != static_cast<ssize_t>(contents.length())) {
    // Delete the file. No shortuct is better than corrupted one. Use unlinkat
    // to make sure we're deleting the file in the directory we think we are.
    // Even if an attacker manager to put something other at
    // |shortcut_filename| we'll just undo his action.
    unlinkat(desktop_fd, shortcut_filename.value().c_str(), 0);
  }

  if (IGNORE_EINTR(close(desktop_fd)) < 0)
    PLOG(ERROR) << "close";

  return true;
}

void DeleteShortcutOnDesktop(const base::FilePath& shortcut_filename) {
  base::FilePath desktop_path;
  if (PathService::Get(base::DIR_USER_DESKTOP, &desktop_path))
    base::DeleteFile(desktop_path.Append(shortcut_filename), false);
}

// Creates a shortcut with |shortcut_filename| and |contents| in the system
// applications menu. If |directory_filename| is non-empty, creates a sub-menu
// with |directory_filename| and |directory_contents|, and stores the shortcut
// under the sub-menu.
bool CreateShortcutInApplicationsMenu(const base::FilePath& shortcut_filename,
                                      const std::string& contents,
                                      const base::FilePath& directory_filename,
                                      const std::string& directory_contents) {
  base::ScopedTempDir temp_dir;
  if (!temp_dir.CreateUniqueTempDir())
    return false;

  base::FilePath temp_directory_path;
  if (!directory_filename.empty()) {
    temp_directory_path = temp_dir.path().Append(directory_filename);

    int bytes_written = file_util::WriteFile(temp_directory_path,
                                             directory_contents.data(),
                                             directory_contents.length());

    if (bytes_written != static_cast<int>(directory_contents.length()))
      return false;
  }

  base::FilePath temp_file_path = temp_dir.path().Append(shortcut_filename);

  int bytes_written = file_util::WriteFile(temp_file_path, contents.data(),
                                           contents.length());

  if (bytes_written != static_cast<int>(contents.length()))
    return false;

  std::vector<std::string> argv;
  argv.push_back("xdg-desktop-menu");
  argv.push_back("install");

  // Always install in user mode, even if someone runs the browser as root
  // (people do that).
  argv.push_back("--mode");
  argv.push_back("user");

  // If provided, install the shortcut file inside the given directory.
  if (!directory_filename.empty())
    argv.push_back(temp_directory_path.value());
  argv.push_back(temp_file_path.value());
  int exit_code;
  LaunchXdgUtility(argv, &exit_code);
  return exit_code == 0;
}

void DeleteShortcutInApplicationsMenu(
    const base::FilePath& shortcut_filename,
    const base::FilePath& directory_filename) {
  std::vector<std::string> argv;
  argv.push_back("xdg-desktop-menu");
  argv.push_back("uninstall");

  // Uninstall in user mode, to match the install.
  argv.push_back("--mode");
  argv.push_back("user");

  // The file does not need to exist anywhere - xdg-desktop-menu will uninstall
  // items from the menu with a matching name.
  // If |directory_filename| is supplied, this will also remove the item from
  // the directory, and remove the directory if it is empty.
  if (!directory_filename.empty())
    argv.push_back(directory_filename.value());
  argv.push_back(shortcut_filename.value());
  int exit_code;
  LaunchXdgUtility(argv, &exit_code);
}

// Quote a string such that it appears as one verbatim argument for the Exec
// key in a desktop file.
std::string QuoteArgForDesktopFileExec(const std::string& arg) {
  // http://standards.freedesktop.org/desktop-entry-spec/latest/ar01s06.html

  // Quoting is only necessary if the argument has a reserved character.
  if (arg.find_first_of(" \t\n\"'\\><~|&;$*?#()`") == std::string::npos)
    return arg;  // No quoting necessary.

  std::string quoted = "\"";
  for (size_t i = 0; i < arg.size(); ++i) {
    // Note that the set of backslashed characters is smaller than the
    // set of reserved characters.
    switch (arg[i]) {
      case '"':
      case '`':
      case '$':
      case '\\':
        quoted += '\\';
        break;
    }
    quoted += arg[i];
  }
  quoted += '"';

  return quoted;
}

const char kDesktopEntry[] = "Desktop Entry";

const char kXdgOpenShebang[] = "#!/usr/bin/env xdg-open";

const char kXdgSettings[] = "xdg-settings";
const char kXdgSettingsDefaultBrowser[] = "default-web-browser";
const char kXdgSettingsDefaultSchemeHandler[] = "default-url-scheme-handler";

const char kDirectoryFilename[] = "chrome-apps.directory";

}  // namespace

namespace {

// Utility function to get the path to the version of a script shipped with
// Chrome. |script| gives the name of the script. |chrome_version| returns the
// path to the Chrome version of the script, and the return value of the
// function is true if the function is successful and the Chrome version is
// not the script found on the PATH.
bool GetChromeVersionOfScript(const std::string& script,
                               std::string* chrome_version) {
  // Get the path to the Chrome version.
  base::FilePath chrome_dir;
  if (!PathService::Get(base::DIR_EXE, &chrome_dir))
    return false;

  base::FilePath chrome_version_path = chrome_dir.Append(script);
  *chrome_version = chrome_version_path.value();

  // Check if this is different to the one on path.
  std::vector<std::string> argv;
  argv.push_back("which");
  argv.push_back(script);
  std::string path_version;
  if (base::GetAppOutput(CommandLine(argv), &path_version)) {
    // Remove trailing newline
    path_version.erase(path_version.length() - 1, 1);
    base::FilePath path_version_path(path_version);
    return (chrome_version_path != path_version_path);
  }
  return false;
}

// Value returned by xdg-settings if it can't understand our request.
const int EXIT_XDG_SETTINGS_SYNTAX_ERROR = 1;

// We delegate the difficulty of setting the default browser and default url
// scheme handler in Linux desktop environments to an xdg utility, xdg-settings.

// When calling this script we first try to use the script on PATH. If that
// fails we then try to use the script that we have included. This gives
// scripts on the system priority over ours, as distribution vendors may have
// tweaked the script, but still allows our copy to be used if the script on the
// system fails, as the system copy may be missing capabilities of the Chrome
// copy.

// If |protocol| is empty this function sets Chrome as the default browser,
// otherwise it sets Chrome as the default handler application for |protocol|.
bool SetDefaultWebClient(const std::string& protocol) {
#if defined(OS_CHROMEOS)
  return true;
#else
  scoped_ptr<base::Environment> env(base::Environment::Create());

  std::vector<std::string> argv;
  argv.push_back(kXdgSettings);
  argv.push_back("set");
  if (protocol.empty()) {
    argv.push_back(kXdgSettingsDefaultBrowser);
  } else {
    argv.push_back(kXdgSettingsDefaultSchemeHandler);
    argv.push_back(protocol);
  }
  argv.push_back(ShellIntegrationLinux::GetDesktopName(env.get()));

  int exit_code;
  bool ran_ok = LaunchXdgUtility(argv, &exit_code);
  if (ran_ok && exit_code == EXIT_XDG_SETTINGS_SYNTAX_ERROR) {
    if (GetChromeVersionOfScript(kXdgSettings, &argv[0])) {
      ran_ok = LaunchXdgUtility(argv, &exit_code);
    }
  }

  return ran_ok && exit_code == EXIT_SUCCESS;
#endif
}

// If |protocol| is empty this function checks if Chrome is the default browser,
// otherwise it checks if Chrome is the default handler application for
// |protocol|.
ShellIntegration::DefaultWebClientState GetIsDefaultWebClient(
    const std::string& protocol) {
#if defined(OS_CHROMEOS)
  return ShellIntegration::UNKNOWN_DEFAULT;
#else
  base::ThreadRestrictions::AssertIOAllowed();

  scoped_ptr<base::Environment> env(base::Environment::Create());

  std::vector<std::string> argv;
  argv.push_back(kXdgSettings);
  argv.push_back("check");
  if (protocol.empty()) {
    argv.push_back(kXdgSettingsDefaultBrowser);
  } else {
    argv.push_back(kXdgSettingsDefaultSchemeHandler);
    argv.push_back(protocol);
  }
  argv.push_back(ShellIntegrationLinux::GetDesktopName(env.get()));

  std::string reply;
  int success_code;
  bool ran_ok = base::GetAppOutputWithExitCode(CommandLine(argv), &reply,
                                               &success_code);
  if (ran_ok && success_code == EXIT_XDG_SETTINGS_SYNTAX_ERROR) {
    if (GetChromeVersionOfScript(kXdgSettings, &argv[0])) {
      ran_ok = base::GetAppOutputWithExitCode(CommandLine(argv), &reply,
                                              &success_code);
    }
  }

  if (!ran_ok || success_code != EXIT_SUCCESS) {
    // xdg-settings failed: we can't determine or set the default browser.
    return ShellIntegration::UNKNOWN_DEFAULT;
  }

  // Allow any reply that starts with "yes".
  return (reply.find("yes") == 0) ? ShellIntegration::IS_DEFAULT :
                                    ShellIntegration::NOT_DEFAULT;
#endif
}

// Get the value of NoDisplay from the [Desktop Entry] section of a .desktop
// file, given in |shortcut_contents|. If the key is not found, returns false.
bool GetNoDisplayFromDesktopFile(const std::string& shortcut_contents) {
  // An empty file causes a crash with glib <= 2.32, so special case here.
  if (shortcut_contents.empty())
    return false;

  GKeyFile* key_file = g_key_file_new();
  GError* err = NULL;
  if (!g_key_file_load_from_data(key_file, shortcut_contents.c_str(),
                                 shortcut_contents.size(), G_KEY_FILE_NONE,
                                 &err)) {
    LOG(WARNING) << "Unable to read desktop file template: " << err->message;
    g_error_free(err);
    g_key_file_free(key_file);
    return false;
  }

  bool nodisplay = false;
  char* nodisplay_c_string = g_key_file_get_string(key_file, kDesktopEntry,
                                                   "NoDisplay", &err);
  if (nodisplay_c_string) {
    if (!g_strcmp0(nodisplay_c_string, "true"))
      nodisplay = true;
    g_free(nodisplay_c_string);
  } else {
    g_error_free(err);
  }

  g_key_file_free(key_file);
  return nodisplay;
}

// Gets the path to the Chrome executable or wrapper script.
// Returns an empty path if the executable path could not be found.
base::FilePath GetChromeExePath() {
  // Try to get the name of the wrapper script that launched Chrome.
  scoped_ptr<base::Environment> environment(base::Environment::Create());
  std::string wrapper_script;
  if (environment->GetVar("CHROME_WRAPPER", &wrapper_script)) {
    return base::FilePath(wrapper_script);
  }

  // Just return the name of the executable path for Chrome.
  base::FilePath chrome_exe_path;
  PathService::Get(base::FILE_EXE, &chrome_exe_path);
  return chrome_exe_path;
}

} // namespace

// static
ShellIntegration::DefaultWebClientSetPermission
    ShellIntegration::CanSetAsDefaultBrowser() {
  return SET_DEFAULT_UNATTENDED;
}

// static
bool ShellIntegration::SetAsDefaultBrowser() {
  return SetDefaultWebClient(std::string());
}

// static
bool ShellIntegration::SetAsDefaultProtocolClient(const std::string& protocol) {
  return SetDefaultWebClient(protocol);
}

// static
ShellIntegration::DefaultWebClientState ShellIntegration::GetDefaultBrowser() {
  return GetIsDefaultWebClient(std::string());
}

// static
std::string ShellIntegration::GetApplicationForProtocol(const GURL& url) {
  return std::string("xdg-open");
}

// static
ShellIntegration::DefaultWebClientState
ShellIntegration::IsDefaultProtocolClient(const std::string& protocol) {
  return GetIsDefaultWebClient(protocol);
}

// static
bool ShellIntegration::IsFirefoxDefaultBrowser() {
  std::vector<std::string> argv;
  argv.push_back(kXdgSettings);
  argv.push_back("get");
  argv.push_back(kXdgSettingsDefaultBrowser);

  std::string browser;
  // We don't care about the return value here.
  base::GetAppOutput(CommandLine(argv), &browser);
  return browser.find("irefox") != std::string::npos;
}

namespace ShellIntegrationLinux {

bool GetDataWriteLocation(base::Environment* env, base::FilePath* search_path) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));

  std::string xdg_data_home;
  std::string home;
  if (env->GetVar("XDG_DATA_HOME", &xdg_data_home) && !xdg_data_home.empty()) {
    *search_path = base::FilePath(xdg_data_home);
    return true;
  } else if (env->GetVar("HOME", &home) && !home.empty()) {
    *search_path = base::FilePath(home).Append(".local").Append("share");
    return true;
  }
  return false;
}

std::vector<base::FilePath> GetDataSearchLocations(base::Environment* env) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));

  std::vector<base::FilePath> search_paths;

  base::FilePath write_location;
  if (GetDataWriteLocation(env, &write_location))
    search_paths.push_back(write_location);

  std::string xdg_data_dirs;
  if (env->GetVar("XDG_DATA_DIRS", &xdg_data_dirs) && !xdg_data_dirs.empty()) {
    base::StringTokenizer tokenizer(xdg_data_dirs, ":");
    while (tokenizer.GetNext()) {
      base::FilePath data_dir(tokenizer.token());
      search_paths.push_back(data_dir);
    }
  } else {
    search_paths.push_back(base::FilePath("/usr/local/share"));
    search_paths.push_back(base::FilePath("/usr/share"));
  }

  return search_paths;
}

std::string GetProgramClassName() {
  DCHECK(CommandLine::InitializedForCurrentProcess());
  // Get the res_name component from argv[0].
  const CommandLine* command_line = CommandLine::ForCurrentProcess();
  std::string class_name = command_line->GetProgram().BaseName().value();
  if (!class_name.empty())
    class_name[0] = base::ToUpperASCII(class_name[0]);
  return class_name;
}

std::string GetDesktopName(base::Environment* env) {
#if defined(GOOGLE_CHROME_BUILD)
  return "google-chrome.desktop";
#else  // CHROMIUM_BUILD
  // Allow $CHROME_DESKTOP to override the built-in value, so that development
  // versions can set themselves as the default without interfering with
  // non-official, packaged versions using the built-in value.
  std::string name;
  if (env->GetVar("CHROME_DESKTOP", &name) && !name.empty())
    return name;
  return "chromium-browser.desktop";
#endif
}

std::string GetIconName() {
#if defined(GOOGLE_CHROME_BUILD)
  return "google-chrome";
#else  // CHROMIUM_BUILD
  return "chromium-browser";
#endif
}

ShellIntegration::ShortcutLocations GetExistingShortcutLocations(
    base::Environment* env,
    const base::FilePath& profile_path,
    const std::string& extension_id) {
  base::FilePath desktop_path;
  // If Get returns false, just leave desktop_path empty.
  PathService::Get(base::DIR_USER_DESKTOP, &desktop_path);
  return GetExistingShortcutLocations(env, profile_path, extension_id,
                                      desktop_path);
}

ShellIntegration::ShortcutLocations GetExistingShortcutLocations(
    base::Environment* env,
    const base::FilePath& profile_path,
    const std::string& extension_id,
    const base::FilePath& desktop_path) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));

  base::FilePath shortcut_filename = GetExtensionShortcutFilename(
      profile_path, extension_id);
  DCHECK(!shortcut_filename.empty());
  ShellIntegration::ShortcutLocations locations;

  // Determine whether there is a shortcut on desktop.
  if (!desktop_path.empty()) {
    locations.on_desktop =
        base::PathExists(desktop_path.Append(shortcut_filename));
  }

  // Determine whether there is a shortcut in the applications directory.
  std::string shortcut_contents;
  if (GetExistingShortcutContents(env, shortcut_filename, &shortcut_contents)) {
    // Whether this counts as "hidden" or "APP_MENU_LOCATION_SUBDIR_CHROMEAPPS"
    // depends on whether it contains NoDisplay=true. Since these shortcuts are
    // for apps, they are always in the "Chrome Apps" directory.
    if (GetNoDisplayFromDesktopFile(shortcut_contents)) {
      locations.hidden = true;
    } else {
      locations.applications_menu_location =
          ShellIntegration::APP_MENU_LOCATION_SUBDIR_CHROMEAPPS;
    }
  }

  return locations;
}

bool GetExistingShortcutContents(base::Environment* env,
                                 const base::FilePath& desktop_filename,
                                 std::string* output) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));

  std::vector<base::FilePath> search_paths = GetDataSearchLocations(env);

  for (std::vector<base::FilePath>::const_iterator i = search_paths.begin();
       i != search_paths.end(); ++i) {
    base::FilePath path = i->Append("applications").Append(desktop_filename);
    VLOG(1) << "Looking for desktop file in " << path.value();
    if (base::PathExists(path)) {
      VLOG(1) << "Found desktop file at " << path.value();
      return base::ReadFileToString(path, output);
    }
  }

  return false;
}

base::FilePath GetWebShortcutFilename(const GURL& url) {
  // Use a prefix, because xdg-desktop-menu requires it.
  std::string filename =
      std::string(chrome::kBrowserProcessExecutableName) + "-" + url.spec();
  file_util::ReplaceIllegalCharactersInPath(&filename, '_');

  base::FilePath desktop_path;
  if (!PathService::Get(base::DIR_USER_DESKTOP, &desktop_path))
    return base::FilePath();

  base::FilePath filepath = desktop_path.Append(filename);
  base::FilePath alternative_filepath(filepath.value() + ".desktop");
  for (size_t i = 1; i < 100; ++i) {
    if (base::PathExists(base::FilePath(alternative_filepath))) {
      alternative_filepath = base::FilePath(
          filepath.value() + "_" + base::IntToString(i) + ".desktop");
    } else {
      return base::FilePath(alternative_filepath).BaseName();
    }
  }

  return base::FilePath();
}

base::FilePath GetExtensionShortcutFilename(const base::FilePath& profile_path,
                                            const std::string& extension_id) {
  DCHECK(!extension_id.empty());

  // Use a prefix, because xdg-desktop-menu requires it.
  std::string filename(chrome::kBrowserProcessExecutableName);
  filename.append("-")
      .append(extension_id)
      .append("-")
      .append(profile_path.BaseName().value());
  file_util::ReplaceIllegalCharactersInPath(&filename, '_');
  // Spaces in filenames break xdg-desktop-menu
  // (see https://bugs.freedesktop.org/show_bug.cgi?id=66605).
  base::ReplaceChars(filename, " ", "_", &filename);
  return base::FilePath(filename.append(".desktop"));
}

std::vector<base::FilePath> GetExistingProfileShortcutFilenames(
    const base::FilePath& profile_path,
    const base::FilePath& directory) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
  // Use a prefix, because xdg-desktop-menu requires it.
  std::string prefix(chrome::kBrowserProcessExecutableName);
  prefix.append("-");
  std::string suffix("-");
  suffix.append(profile_path.BaseName().value());
  file_util::ReplaceIllegalCharactersInPath(&suffix, '_');
  // Spaces in filenames break xdg-desktop-menu
  // (see https://bugs.freedesktop.org/show_bug.cgi?id=66605).
  base::ReplaceChars(suffix, " ", "_", &suffix);
  std::string glob = prefix + "*" + suffix + ".desktop";

  base::FileEnumerator files(directory, false, base::FileEnumerator::FILES,
                             glob);
  base::FilePath shortcut_file = files.Next();
  std::vector<base::FilePath> shortcut_paths;
  while (!shortcut_file.empty()) {
    shortcut_paths.push_back(shortcut_file.BaseName());
    shortcut_file = files.Next();
  }
  return shortcut_paths;
}

std::string GetDesktopFileContents(
    const base::FilePath& chrome_exe_path,
    const std::string& app_name,
    const GURL& url,
    const std::string& extension_id,
    const base::FilePath& extension_path,
    const base::string16& title,
    const std::string& icon_name,
    const base::FilePath& profile_path,
    bool no_display) {
  // Although not required by the spec, Nautilus on Ubuntu Karmic creates its
  // launchers with an xdg-open shebang. Follow that convention.
  std::string output_buffer = std::string(kXdgOpenShebang) + "\n";

  // See http://standards.freedesktop.org/desktop-entry-spec/latest/
  GKeyFile* key_file = g_key_file_new();

  // Set keys with fixed values.
  g_key_file_set_string(key_file, kDesktopEntry, "Version", "1.0");
  g_key_file_set_string(key_file, kDesktopEntry, "Terminal", "false");
  g_key_file_set_string(key_file, kDesktopEntry, "Type", "Application");

  // Set the "Name" key.
  std::string final_title = base::UTF16ToUTF8(title);
  // Make sure no endline characters can slip in and possibly introduce
  // additional lines (like Exec, which makes it a security risk). Also
  // use the URL as a default when the title is empty.
  if (final_title.empty() ||
      final_title.find("\n") != std::string::npos ||
      final_title.find("\r") != std::string::npos) {
    final_title = url.spec();
  }
  g_key_file_set_string(key_file, kDesktopEntry, "Name", final_title.c_str());

  // Set the "Exec" key.
  std::string final_path = chrome_exe_path.value();
  CommandLine cmd_line(CommandLine::NO_PROGRAM);
  cmd_line = ShellIntegration::CommandLineArgsForLauncher(
      url, extension_id, profile_path);
  const CommandLine::SwitchMap& switch_map = cmd_line.GetSwitches();
  for (CommandLine::SwitchMap::const_iterator i = switch_map.begin();
       i != switch_map.end(); ++i) {
    if (i->second.empty()) {
      final_path += " --" + i->first;
    } else {
      final_path += " " + QuoteArgForDesktopFileExec("--" + i->first +
                                                     "=" + i->second);
    }
  }

  g_key_file_set_string(key_file, kDesktopEntry, "Exec", final_path.c_str());

  // Set the "Icon" key.
  if (!icon_name.empty()) {
    g_key_file_set_string(key_file, kDesktopEntry, "Icon", icon_name.c_str());
  } else {
    g_key_file_set_string(key_file, kDesktopEntry, "Icon",
                          GetIconName().c_str());
  }

  // Set the "NoDisplay" key.
  if (no_display)
    g_key_file_set_string(key_file, kDesktopEntry, "NoDisplay", "true");

  std::string wmclass = web_app::GetWMClassFromAppName(app_name);
  g_key_file_set_string(key_file, kDesktopEntry, "StartupWMClass",
                        wmclass.c_str());

  gsize length = 0;
  gchar* data_dump = g_key_file_to_data(key_file, &length, NULL);
  if (data_dump) {
    // If strlen(data_dump[0]) == 0, this check will fail.
    if (data_dump[0] == '\n') {
      // Older versions of glib produce a leading newline. If this is the case,
      // remove it to avoid double-newline after the shebang.
      output_buffer += (data_dump + 1);
    } else {
      output_buffer += data_dump;
    }
    g_free(data_dump);
  }

  g_key_file_free(key_file);
  return output_buffer;
}

std::string GetDirectoryFileContents(const base::string16& title,
                                     const std::string& icon_name) {
  // See http://standards.freedesktop.org/desktop-entry-spec/latest/
  GKeyFile* key_file = g_key_file_new();

  g_key_file_set_string(key_file, kDesktopEntry, "Version", "1.0");
  g_key_file_set_string(key_file, kDesktopEntry, "Type", "Directory");
  std::string final_title = base::UTF16ToUTF8(title);
  g_key_file_set_string(key_file, kDesktopEntry, "Name", final_title.c_str());
  if (!icon_name.empty()) {
    g_key_file_set_string(key_file, kDesktopEntry, "Icon", icon_name.c_str());
  } else {
    g_key_file_set_string(key_file, kDesktopEntry, "Icon",
                          GetIconName().c_str());
  }

  gsize length = 0;
  gchar* data_dump = g_key_file_to_data(key_file, &length, NULL);
  std::string output_buffer;
  if (data_dump) {
    // If strlen(data_dump[0]) == 0, this check will fail.
    if (data_dump[0] == '\n') {
      // Older versions of glib produce a leading newline. If this is the case,
      // remove it to avoid double-newline after the shebang.
      output_buffer += (data_dump + 1);
    } else {
      output_buffer += data_dump;
    }
    g_free(data_dump);
  }

  g_key_file_free(key_file);
  return output_buffer;
}

bool CreateDesktopShortcut(
    const ShellIntegration::ShortcutInfo& shortcut_info,
    const ShellIntegration::ShortcutLocations& creation_locations) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));

  base::FilePath shortcut_filename;
  if (!shortcut_info.extension_id.empty()) {
    shortcut_filename = GetExtensionShortcutFilename(
        shortcut_info.profile_path, shortcut_info.extension_id);
    // For extensions we do not want duplicate shortcuts. So, delete any that
    // already exist and replace them.
    if (creation_locations.on_desktop)
      DeleteShortcutOnDesktop(shortcut_filename);
    // The 'applications_menu_location' and 'hidden' locations are actually the
    // same place ('applications').
    if (creation_locations.applications_menu_location !=
            ShellIntegration::APP_MENU_LOCATION_NONE ||
        creation_locations.hidden)
      DeleteShortcutInApplicationsMenu(shortcut_filename, base::FilePath());
  } else {
    shortcut_filename = GetWebShortcutFilename(shortcut_info.url);
  }
  if (shortcut_filename.empty())
    return false;

  std::string icon_name = CreateShortcutIcon(shortcut_info, shortcut_filename);

  std::string app_name =
      web_app::GenerateApplicationNameFromInfo(shortcut_info);

  bool success = true;

  base::FilePath chrome_exe_path = GetChromeExePath();
  if (chrome_exe_path.empty()) {
    LOG(WARNING) << "Could not get executable path.";
    return false;
  }

  if (creation_locations.on_desktop) {
    std::string contents = ShellIntegrationLinux::GetDesktopFileContents(
        chrome_exe_path,
        app_name,
        shortcut_info.url,
        shortcut_info.extension_id,
        shortcut_info.extension_path,
        shortcut_info.title,
        icon_name,
        shortcut_info.profile_path,
        false);
    success = CreateShortcutOnDesktop(shortcut_filename, contents);
  }

  if (creation_locations.applications_menu_location !=
          ShellIntegration::APP_MENU_LOCATION_NONE ||
      creation_locations.hidden) {
    base::FilePath directory_filename;
    std::string directory_contents;
    switch (creation_locations.applications_menu_location) {
      case ShellIntegration::APP_MENU_LOCATION_NONE:
      case ShellIntegration::APP_MENU_LOCATION_ROOT:
        break;
      case ShellIntegration::APP_MENU_LOCATION_SUBDIR_CHROMEAPPS:
        directory_filename = base::FilePath(kDirectoryFilename);
        directory_contents = ShellIntegrationLinux::GetDirectoryFileContents(
            ShellIntegration::GetAppShortcutsSubdirName(), "");
        break;
      default:
        NOTREACHED();
        break;
    }
    // Set NoDisplay=true if hidden but not in the applications menu. This will
    // hide the application from user-facing menus.
    std::string contents = ShellIntegrationLinux::GetDesktopFileContents(
        chrome_exe_path,
        app_name,
        shortcut_info.url,
        shortcut_info.extension_id,
        shortcut_info.extension_path,
        shortcut_info.title,
        icon_name,
        shortcut_info.profile_path,
        creation_locations.applications_menu_location ==
            ShellIntegration::APP_MENU_LOCATION_NONE);
    success = CreateShortcutInApplicationsMenu(
        shortcut_filename, contents, directory_filename, directory_contents) &&
        success;
  }

  return success;
}

void DeleteDesktopShortcuts(const base::FilePath& profile_path,
                            const std::string& extension_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));

  base::FilePath shortcut_filename = GetExtensionShortcutFilename(
      profile_path, extension_id);
  DCHECK(!shortcut_filename.empty());

  DeleteShortcutOnDesktop(shortcut_filename);
  // Delete shortcuts from |kDirectoryFilename|.
  // Note that it is possible that shortcuts were not created in the Chrome Apps
  // directory. It doesn't matter: this will still delete the shortcut even if
  // it isn't in the directory.
  DeleteShortcutInApplicationsMenu(shortcut_filename,
                                   base::FilePath(kDirectoryFilename));
}

void DeleteAllDesktopShortcuts(const base::FilePath& profile_path) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));

  scoped_ptr<base::Environment> env(base::Environment::Create());

  // Delete shortcuts from Desktop.
  base::FilePath desktop_path;
  if (PathService::Get(base::DIR_USER_DESKTOP, &desktop_path)) {
    std::vector<base::FilePath> shortcut_filenames_desktop =
        GetExistingProfileShortcutFilenames(profile_path, desktop_path);
    for (std::vector<base::FilePath>::const_iterator it =
         shortcut_filenames_desktop.begin();
         it != shortcut_filenames_desktop.end(); ++it) {
      DeleteShortcutOnDesktop(*it);
    }
  }

  // Delete shortcuts from |kDirectoryFilename|.
  base::FilePath applications_menu;
  if (GetDataWriteLocation(env.get(), &applications_menu)) {
    applications_menu = applications_menu.AppendASCII("applications");
    std::vector<base::FilePath> shortcut_filenames_app_menu =
        GetExistingProfileShortcutFilenames(profile_path, applications_menu);
    for (std::vector<base::FilePath>::const_iterator it =
         shortcut_filenames_app_menu.begin();
         it != shortcut_filenames_app_menu.end(); ++it) {
      DeleteShortcutInApplicationsMenu(*it,
                                       base::FilePath(kDirectoryFilename));
    }
  }
}

}  // namespace ShellIntegrationLinux
