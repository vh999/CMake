/*============================================================================
  CMake - Cross Platform Makefile Generator
  Copyright 2004-2009 Kitware, Inc.
  Copyright 2004 Alexander Neundorf (neundorf@kde.org)

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
#ifndef cmExtraSlickEditGenerator_h
#define cmExtraSlickEditGenerator_h

#include "cmExternalMakefileProjectGenerator.h"
#include "cmTargetDepend.h"
#include "cmSourceFile.h"

class cmLocalGenerator;
class cmMakefile;
class cmTarget;
class cmGeneratedFileStream;
class cmGeneratorTarget;

/** \class cmExtraSlickEditGenerator
 * \brief Write SlickEdit  project files for Makefile based projects
 */
class cmExtraSlickEditGenerator : public cmExternalMakefileProjectGenerator {
public:
  typedef std::map<std::string, std::vector<std::string> > MapSourceFileFlags;
  cmExtraSlickEditGenerator();

  virtual std::string GetName() const
  { return cmExtraSlickEditGenerator::GetActualName();}
  static std::string GetActualName()
  { return "SlickEdit";}
  static cmExternalMakefileProjectGenerator* New()
  { return new cmExtraSlickEditGenerator; }
  /** Get the documentation entry for this generator.  */
  virtual void GetDocumentation(cmDocumentationEntry &entry,
                                const std::string &fullName) const;

  virtual void Generate();
private:
  bool m_include_wildcards;

  std::set<std::string> m_project_files;

  void DebugOutput();
  void GenerateTargets();
  void GenerateProjects();

  void CreateVPW(const std::vector<cmLocalGenerator *> &lgs);

  void WriteBuildAllProject(const cmMakefile *mf);
  void WriteTargetProject(const cmTarget &target);
  void WriteTargetCommand(cmGeneratedFileStream& fout,
                          std::string command,
                          std::string name,
                          std::string caption,
                          bool buildFirst);

  void WriteProjectFileV2(const cmMakefile *mf,
                          std::set<std::string>& source_files,
                          std::set<std::string>& include_paths);

  void WriteIncludePathWildcards(cmGeneratedFileStream& fout,
                                 const cmMakefile *mf,
                                 std::string& project_path,
                                 std::vector<std::string>& include_list);

  std::string GetBuildCommand(const cmMakefile* mf, const std::string target) const;
  
  void QueryCompiler(const cmMakefile *mf);


};

#endif
