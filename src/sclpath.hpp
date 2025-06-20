/*  sclpath.hpp
 *  Path class declaration for SCL
 */

#ifndef SCL_PATH_H
#define SCL_PATH_H

#include "sclcore.hpp"
#include <vector>

namespace scl {
class path : public string {
  path &fixendsplit();

 public:
  path();
  path (const string &rhs);
  path (const char *rhs);

  /**
   * @return Full OS path of this path.
   *
   */
  path resolve() const;

  bool haspath (const path &path) const;

  /**
   * @brief
   *
   */
  path relative (const path &from = ".") const;

  /**
   * @return Parent directory of this path.
   * Ex: foo/bar/fun.txt => foo/bar
   *
   */
  path parentpath() const;

  /**
   * @return  Returns the filename of this path.
   *
   */
  path filename() const;

  /**
   * @return Extension of this path.
   */
  string extension() const;

  /**
   * @return Stem component of this path.
   * Ex: foo/bar/fun.txt => fun
   */
  path stem() const;

  /**
   * @brief Returns whether or not this path contains a wildcard (*).
   *
   * @return <b>true</b> if this path contains a wildcard, <b>false</b> if
   * otherwise.
   */
  bool iswild() const;

  /**
   * @brief Splits this path into its individual compontents.
   * Ex: foo/bar/fun.txt => {foo, bar, fun.txt}
   *
   * @return   Vector of each component of this path.
   */
  std::vector<path> split() const;

  /**
   * @brief Returns whether or not a directory or file exists at this path.
   *
   * @return   <b>true</b> if a directory or file exists, <b>false</b> if
   * otherwise.
   */
  bool exists() const;

  /**
   * @return   Whether or not this path represents a file.
   */
  bool isfile() const;

  /**
   * @return   Whether or not this path represents a directory.
   */
  bool isdirectory() const;

  /**
   * @brief
   *
   * @return  Whether or not this path is an absolute path.
   */
  bool isabsolute() const;

  /**
   * @brief Returns the write time of the file at this path.
   *
   * @return   Write time in seconds since UNIX epoch.
   */
  long long wtime() const;

  /**
   * @brief Deletes this file from the file system.
   * If this path isnt a file, nothing happens.
   * @note If you want to remove a directory, or glob expression, use the static
   * scl::path::remove
   *
   */
  void remove() const;

  path &replaceFilename (const path &nFile);

  path &replaceExtension (const path &nExt);

  path &replaceStem (const path &nName);

  /**
   * @return   Current working directory of this program.
   */
  static path cwd();

  /**
   * @return   Directory that this executable resides in.
   */
  static path execdir();

  /**
   * @brief Changes the cwd of this executable.
   *
   * @param path  Path to cd into.
   * @return  <b>true</b> if the change in cwd was successful.
   */
  static bool chdir (const path &path);

  /**
   * @brief Creates a directory.
   * @note This function will create any necessary directories in the given
   * path.
   *
   * @param path  Path of the directory to create.
   * @return   <b>true</b> if the creation was succesful, <b>false</b> if
   * otherwise.
   */
  static bool mkdir (const path &path);
  static bool mkdir (std::vector<path> paths);

  /**
   * @brief Recursively removes any files matching this pattern
   *
   * @param pattern  Glob expression to use.
   */
  static void remove (const path &pattern);

  /**
   * @brief Copies a file from `from` to `to`.
   *
   * @param from  Source path.
   * @param to  Destination path.
   * @return  True if the copy was successful, false if otherwise.
   */
  static bool copyfile (const path &from, const path &to);

  /**
   * @brief Moves a file from `from` to `to`.
   *
   * @param from  Source path.
   * @param to  Destination path.
   * @return  True if the move was successful, false if otherwise.
   */
  static bool movefile (const path &from, const path &to);

  /**
   * @brief Returns a vector of any existing files that match a glob pattern.
   * Ex: .\*.txt: search for any .txt file, **\*.txt: recursive search for any
   * .txt file.
   * @note Multiple widcards, and multiple recursive wildcards can be present in
   * the glob expression.
   *
   * @param pattern  Glob expression to use.
   * @return   Vector of any files that matched the glob expression.
   */
  static std::vector<path> glob (const string &pattern);

  static path join (std::vector<path> components, bool ignoreback = false);

  path &join (const path &rhs, bool relative = true);

  /**
   * @brief Returns this path, with a component appended, seperated by the host
   * systems directory seperator.
   *
   * @param rhs  Component to append.
   * @return  Composite path.
   */
  path operator/ (const path &rhs) const;

  /**
   * @brief Appends a component, seperated by the host
   * systems directory seperator.
   *
   * @param rhs  Component to append.
   */
  path &operator/= (const path &rhs);
};
} // namespace scl

#endif