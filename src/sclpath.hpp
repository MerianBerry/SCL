/*  sclpath.hpp
 *  Path class declaration for SCL
 */

#ifndef SCL_PATH_H
#define SCL_PATH_H

#include "sclcore.hpp"
#include <vector>

namespace scl {
class path : public string {
 public:
  path();
  path (string const &rhs);
  path (char const *rhs);

  /**
   * @return Full OS path of this path.
   *
   */
  path resolve() const;

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
   * @return Name component of this path.
   * Ex: foo/bar/fun.txt => fun
   */
  path basename() const;

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
   * @brief Returns the write time of the file at this path.
   *
   * @return   Write time in seconds since UNIX epoch.
   */
  long wtime() const;

  path &replaceFilename (path const &nFile);

  path &replaceExtension (path const &nExt);

  path &replaceBasename (path const &nName);

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
  static bool chdir (path const &path);

  /**
   * @brief Creates a directory.
   * @note This function will create any necessary directories in the given
   * path.
   *
   * @param path  Path of the directory to create.
   * @return   <b>true</b> if the creation was succesful, <b>false</b> if
   * otherwise.
   */
  static bool mkdir (path const &path);
  static bool mkdir (std::vector<path> paths);

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
  static std::vector<string> glob (string const &pattern);

  static path join (std::vector<path> components);

  /**
   * @brief Returns this path, with a component appended, seperated by the host
   * systems directory seperator.
   *
   * @param rhs  Component to append.
   * @return  Composite path.
   */
  path operator/ (path const &rhs) const;

  /**
   * @brief Appends a component, seperated by the host
   * systems directory seperator.
   *
   * @param rhs  Component to append.
   */
  path &operator/= (path const &rhs);
};
} // namespace scl

#endif