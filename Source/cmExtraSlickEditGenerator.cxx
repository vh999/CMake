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
#include "cmExtraSlickEditGenerator.h"
#include "cmake.h"
#include "cmGeneratedFileStream.h"
#include "cmGeneratorTarget.h"
#include "cmGlobalUnixMakefileGenerator3.h"
#include "cmLocalGenerator.h"
#include "cmLocalUnixMakefileGenerator3.h"
#include "cmMakefile.h"
#include "cmSourceFile.h"
#include "cmSystemTools.h"
#include "cmTarget.h"
#include "cmXMLSafe.h"

#include <cmsys/SystemTools.hxx>

//----------------------------------------------------------------------------
void cmExtraSlickEditGenerator
::GetDocumentation(cmDocumentationEntry& entry, const std::string&) const
{
  entry.Name = this->GetName();
  entry.Brief = "Generates SlickEdit project files.";
}

cmExtraSlickEditGenerator::cmExtraSlickEditGenerator()
  : cmExternalMakefileProjectGenerator(), m_include_wildcards(false)
{
#if defined(_WIN32)
  this->SupportedGlobalGenerators.push_back("MinGW Makefiles");
  this->SupportedGlobalGenerators.push_back("NMake Makefiles");
#endif
  this->SupportedGlobalGenerators.push_back("Ninja");
  this->SupportedGlobalGenerators.push_back("Unix Makefiles");
}

void cmExtraSlickEditGenerator::DebugOutput()
{
  const char* vars = this->GlobalGenerator->GetCMakeInstance()->GetState()->GetInitializedCacheValue("CMAKE_SLICKEDIT");
  if (vars)
    {
    std::cout << "CMAKE_SLICKEDIT=" << vars << std::endl;
    }

  for (std::map<std::string, std::vector<cmLocalGenerator *> >::const_iterator
       it = this->GlobalGenerator->GetProjectMap().begin();
       it != this->GlobalGenerator->GetProjectMap().end();
       ++it)
    {
    const std::vector<cmLocalGenerator *>& gen = it->second;

	std::cout << "*** " << it->first << " ***" << std::endl;

	const cmMakefile* mf = gen[0]->GetMakefile();
	std::string proj_name = mf->GetProjectName();
    std::string proj_path = mf->GetCurrentBinaryDirectory();
    std::cout << "MakeFile Project=" << proj_name << std::endl;
    std::cout << "Makefile Path=" << proj_path << std::endl;
    
    for (std::vector<cmLocalGenerator *>::const_iterator i = gen.begin();
         i != gen.end(); ++i)
      {
      const cmMakefile *mf = (*i)->GetMakefile();
      std::string make_name = mf->GetProjectName();
      std::string make_path = mf->GetCurrentBinaryDirectory();
      std::string home_dir = mf->GetHomeDirectory();
      std::string output_dir = mf->GetHomeOutputDirectory();
      std::cout << "  MakeFile Project=" << make_name << std::endl;
      std::cout << "  Makefile Path=" << make_path << std::endl;
      std::cout << "  Home Directory=" << home_dir << std::endl;
      std::cout << "  Output Directory=" << output_dir << std::endl;

      std::vector<std::string> configs;
      mf->GetConfigurations(configs);
      for (std::vector<std::string>::const_iterator ci = configs.begin();
           ci != configs.end(); ++ci)
        {
        std::cout << "  Configurations=" << (*ci) << std::endl;
        }

      const cmTargets& targets = mf->GetTargets();
      for (cmTargets::const_iterator t = targets.begin(); t != targets.end(); ++t)
        {
        std::string name = t->second.GetName();
        std::string path = t->second.GetMakefile()->GetCurrentBinaryDirectory();

        bool show_includes = false;
        switch (t->second.GetType())
          {
          case cmTarget::EXECUTABLE:
          case cmTarget::STATIC_LIBRARY:
          case cmTarget::SHARED_LIBRARY:
          case cmTarget::MODULE_LIBRARY:
          case cmTarget::OBJECT_LIBRARY:
            show_includes = true;
            std::cout << "    Target[" << t->second.GetType() << "]=" << path << " " << name << std::endl;
            break;

          default:
            std::cout << "    Other[" << t->second.GetType() << "]=" << t->second.GetName() << std::endl;
            break;
          }

        std::string source_file;
        std::vector<cmSourceFile *> sources;
        t->second.GetSourceFiles(sources, mf->GetSafeDefinition("CMAKE_BUILD_TYPE"));
        for (std::vector<cmSourceFile *>::const_iterator sfIt = sources.begin(); sfIt != sources.end(); sfIt++)
          {
          cmSourceFile *sf = *sfIt;
          if (sf->GetPropertyAsBool("GENERATED"))
            {
            continue;
            }
          source_file = sf->GetFullPath();
          std::string filename = cmSystemTools::RelativePath(proj_path.c_str(), source_file.c_str());
          std::cout << "    SourceFile=" << filename << std::endl;
          }

        std::string header_extensions;
        const std::vector<std::string>& ext = mf->GetHeaderExtensions();
        for (std::vector<std::string>::const_iterator e = ext.begin(); e != ext.end(); ++e)
          {
          if (!header_extensions.empty()) header_extensions += " ";
          header_extensions += (*e);
          }
        std::cout << "  HeaderExtensions=" << header_extensions << std::endl;

        if (show_includes)
          {
          std::string buildType = mf->GetSafeDefinition("CMAKE_BUILD_TYPE");
          cmGeneratorTarget *gt = this->GlobalGenerator->GetGeneratorTarget(&t->second);
          std::vector<std::string> include_list;
          mf->GetLocalGenerator()->GetIncludeDirectories(include_list, gt, "C", buildType);
          if (include_list.size() > 0)
            {
            for (std::vector<std::string>::const_iterator inc_t = include_list.begin(); inc_t != include_list.end(); ++inc_t)
              {
              //std::string path = cmSystemTools::RelativePath(proj_path.c_str(), (*inc_t).c_str());
              std::cout << "    IncludePath=" << (*inc_t) << std::endl;
              }
            }
          }
        }
      }
    }

  if (this->GlobalGenerator->GetAllTargetName())
    {
    std::cout << "AllTargetName=" << this->GlobalGenerator->GetAllTargetName() << std::endl;
    }
}

void cmExtraSlickEditGenerator::Generate()
{
  std::string generator_args;
  const char* vars = this->GlobalGenerator->GetCMakeInstance()->GetState()->GetInitializedCacheValue("CMAKE_SLICKEDIT");
  if (vars) {
	  generator_args.assign(vars);
  }

  bool debug_output = false;
  bool generate_targets = false;

  std::string arg;
  std::stringstream ss(generator_args);
  while (ss >> arg) {
	  if (arg == "DEBUG") {
		  debug_output = true;
	  }
	  if (arg == "ADDINCLUDEPATHS") {
		  m_include_wildcards = true;
	  }
	  if (arg == "TARGETS") {
		  generate_targets = true;
	  }
  }

  if (generate_targets) {
    GenerateTargets();
  } else {
    GenerateProjects();
  }

  if (debug_output) {
    DebugOutput();
  }
}

void cmExtraSlickEditGenerator::GenerateTargets()
{
  for (std::map<std::string, std::vector<cmLocalGenerator *> >::const_iterator
       it = this->GlobalGenerator->GetProjectMap().begin();
       it != this->GlobalGenerator->GetProjectMap().end();
       ++it)
    {
    this->CreateVPW(it->second);
    }
}

void cmExtraSlickEditGenerator::CreateVPW(const std::vector<cmLocalGenerator *>& lgs)
{
  const cmMakefile *mf = lgs[0]->GetMakefile();
  std::string workspace_path = mf->GetCurrentBinaryDirectory();
  std::string filename = workspace_path + "/" + mf->GetProjectName() + ".vpw";
  std::string rel_filename;

  cmGeneratedFileStream wout(filename.c_str());
  if (!wout)
    {
    return;
    }

  // add workspace header
  wout << "<!DOCTYPE Workspace SYSTEM \"http://www.slickedit.com/dtd/vse/10.0/vpw.dtd\">\n"
    "<Workspace Version=\"10.0\" VendorName=\"SlickEdit\">\n"
    "    <Projects>\n";

  WriteBuildAllProject(mf);
  wout << "		<Project File=\"BUILD_ALL.vpj\"/>\n";

  for (std::vector<cmLocalGenerator *>::const_iterator it = lgs.begin(); it != lgs.end(); ++it)
    {
    mf = (*it)->GetMakefile();
    const cmTargets& targets = mf->GetTargets();
    for (cmTargets::const_iterator t = targets.begin(); t != targets.end(); ++t)
      {
      const cmTarget& target = t->second;
	  std::string path = target.GetMakefile()->GetCurrentBinaryDirectory();
      switch (target.GetType())
        {
        case cmTarget::EXECUTABLE:
        case cmTarget::STATIC_LIBRARY:
        case cmTarget::SHARED_LIBRARY:
        case cmTarget::MODULE_LIBRARY:
        case cmTarget::OBJECT_LIBRARY:
          WriteTargetProject(target);
          filename = path + "/" + target.GetName() + ".vpj";
          rel_filename = cmSystemTools::RelativePath(workspace_path.c_str(), filename.c_str());
          wout << "		<Project File=\"" + rel_filename + "\"/>\n";
          break;
        }
      }
    }
  wout << "    </Projects>\n";
  wout << "</Workspace>\n";
}

void cmExtraSlickEditGenerator::WriteBuildAllProject(const cmMakefile *mf)
{
  std::string project_path = mf->GetCurrentBinaryDirectory();
  std::string project_name = "BUILD_ALL.vpj";
  std::string project_filename = project_path + "/" + project_name;
  if (m_project_files.find(project_filename) != m_project_files.end())
    {
    return;
    }

  std::string make = mf->GetRequiredDefinition("CMAKE_MAKE_PROGRAM");
  std::string makefile = project_path + "/Makefile";
  std::string command;

  cmGeneratedFileStream fout(project_filename.c_str());
  fout << "<!DOCTYPE Project SYSTEM \"http://www.slickedit.com/dtd/vse/10.0/vpj.dtd\">\n"
    "<Project\n"
    "    Version=\"10.0\"\n"
    "    VendorName=\"SlickEdit\"\n"
    "    TemplateName=\"CMake\"\n";
  fout << "    WorkingDir=\".\">\n";


  std::vector<std::string> configs;
  mf->GetConfigurations(configs);
  if (configs.empty())
    {
    configs.push_back("Release");
    }

  for (std::vector<std::string>::const_iterator it = configs.begin(); it != configs.end(); ++it)
    {
    const std::string& config = (*it);
    fout << "    <Config\n";
    fout << "        Name=\"" + config + "\">\n";
    fout << "        <Menu>\n";
    
    command = GetBuildCommand(mf, "all");
    WriteTargetCommand(fout, command, "Build", "&amp;Build", false);

    // clean target
    command = GetBuildCommand(mf, "clean");
    WriteTargetCommand(fout, command, "Clean", "&amp;Clean", false);

    fout << "        </Menu>\n";
    fout << "    </Config>\n";
    }

  fout << "    <Files>\n";
  // write out first cmakefile
  // TBD:   include all list files?  is first one always root cmakefile?
  const char *cmakeRoot = mf->GetDefinition("CMAKE_ROOT");
  const std::vector<std::string>& list_files = mf->GetListFiles();
  if (list_files.size() > 0)
    {
    const std::string& cmakefile = list_files[0];
    std::string source_file = cmSystemTools::RelativePath(project_path.c_str(), cmakefile.c_str());
    fout << "      <F N=\"" + source_file + "\"/>\n";
    }

  fout << "    </Files>\n";
  fout << "</Project>\n";

  m_project_files.insert(project_filename);
}

void cmExtraSlickEditGenerator::WriteTargetCommand(cmGeneratedFileStream& fout,
                                                   std::string command,
                                                   std::string name,
                                                   std::string caption,
                                                   bool buildFirst)
{
  if (command.empty()) return;

  fout << "            <Target\n";
  fout << "                Name=\"" + name + "\"\n";
  fout << "                MenuCaption=\"" + caption + "\"\n";
  fout << "                CaptureOutputWith=\"ProcessBuffer\"\n";
  if (buildFirst)
    {
    fout << "                BuildFirst=\"1\"\n";
    }
  fout << "                Deletable=\"0\"\n";
  fout << "                SaveOption=\"SaveCurrent\"\n";
  fout << "                RunFromDir=\"%rw\">\n";
  fout << "                <Exec CmdLine='" + command + "'/>\n";
  fout << "            </Target>\n";
}

std::string
cmExtraSlickEditGenerator::GetBuildCommand(const cmMakefile* mf, const std::string target) const
{
  std::string generator = mf->GetSafeDefinition("CMAKE_GENERATOR");
  std::string make = mf->GetRequiredDefinition("CMAKE_MAKE_PROGRAM");
  std::string buildCommand = make; // Default
  if ( generator == "NMake Makefiles" ||
       generator == "Ninja" )
    {
    buildCommand = make;
    }

  if (!target.empty())
    {
    if (generator == "Ninja")
      {
      buildCommand += " -v";
      }
    buildCommand += " ";
    buildCommand += target;
    }
  return buildCommand;
}


void cmExtraSlickEditGenerator::WriteIncludePathWildcards(cmGeneratedFileStream& fout,
                                                          const cmMakefile *mf,
                                                          std::string& project_path,
                                                          std::vector<std::string>& include_list)
{
  if (include_list.empty()) return;

  const std::vector<std::string>& ext = mf->GetHeaderExtensions();
  for (std::vector<std::string>::const_iterator it = include_list.begin(); it != include_list.end(); ++it)
    {
    std::string path = cmSystemTools::RelativePath(project_path.c_str(), (*it).c_str());
    for (std::vector<std::string>::const_iterator e = ext.begin(); e != ext.end(); ++e)
      {
          std::string n = path + "/*." + (*e);
          fout << "            <F\n";
          fout << "                N=\"" + n + "\"\n";
          fout << "                Recurse=\"0\"\n";
          fout << "                Excludes=\"\"/>\n";
      }
    }
}

void cmExtraSlickEditGenerator::WriteTargetProject(const cmTarget& target)
{
  const cmMakefile *mf = target.GetMakefile();
  std::string name = target.GetName();
  std::string project_path = mf->GetCurrentBinaryDirectory();
  std::string project_filename = project_path + "/" + name + ".vpj";

  if (m_project_files.find(project_filename) != m_project_files.end())
    {
    return;
    }

  cmGeneratedFileStream fout(project_filename.c_str());
  fout << "<!DOCTYPE Project SYSTEM \"http://www.slickedit.com/dtd/vse/10.0/vpj.dtd\">\n"
    "<Project\n"
    "    Version=\"10.0\"\n"
    "    VendorName=\"SlickEdit\"\n"
    "    TemplateName=\"CMake\"\n";
  fout << "    WorkingDir=\".\">\n";

  // Write Config section
  
  std::vector<std::string> configs;
  mf->GetConfigurations(configs);
  if (configs.empty())
    {
    configs.push_back("Release");
    }

  std::string make = mf->GetRequiredDefinition("CMAKE_MAKE_PROGRAM");
  std::string makefile = project_path + "/Makefile";
  std::string buildType = mf->GetSafeDefinition("CMAKE_BUILD_TYPE");
  for (std::vector<std::string>::const_iterator it = configs.begin(); it != configs.end(); ++it)
    {
    const std::string& config = (*it);
    fout << "    <Config\n";
    fout << "        Name=\"" + config + "\"\n";

    std::string output_file;
    switch (target.GetType())
      {
      case cmTarget::EXECUTABLE:
      case cmTarget::STATIC_LIBRARY:
      case cmTarget::SHARED_LIBRARY:
      case cmTarget::MODULE_LIBRARY:
        output_file = target.GetLocation(buildType);
        break;

      case cmTarget::OBJECT_LIBRARY:
        break;
      }

	std::string path;
	std::string command;

    if (!output_file.empty())
      {
      path = cmSystemTools::RelativePath(project_path.c_str(), output_file.c_str());
      fout << "        OutputFile=\"%rw" + path + "\"\n";
      }

    // TBD: Determine compile config name from env????
    fout << "        CompilerConfigName=\"Latest Version\">\n";
    fout << "        <Menu>\n";

    // build target
    //command.clear();
    command = GetBuildCommand(mf, target.GetName());
    WriteTargetCommand(fout, command, "Build", "&amp;Build", false);
    
    // compile target
    command = GetBuildCommand(mf, "\"%f\"");
    WriteTargetCommand(fout, command, "Compile", "&amp;Compile", false);

    // clean target
    command = GetBuildCommand(mf, "clean");
    WriteTargetCommand(fout, command, "Clean", "&amp;Clean", false);

    // execute target
    switch (target.GetType())
      {
      case cmTarget::EXECUTABLE:
        if (!output_file.empty())
          {
          WriteTargetCommand(fout, "\"%o\"", "Execute", "E&amp;xecute", true);
          }
        break;
      }
    fout << "        </Menu>\n";

    // includes
    cmGeneratorTarget *gt = this->GlobalGenerator->GetGeneratorTarget(&target);
    std::vector<std::string> include_list;

    mf->GetLocalGenerator()->GetIncludeDirectories(include_list, gt, "C", buildType);
    if (include_list.size() > 0)
      {
      fout << "        <Includes>\n";
      for (std::vector<std::string>::const_iterator it = include_list.begin(); it != include_list.end(); ++it)
        {
        path = cmSystemTools::RelativePath(project_path.c_str(), (*it).c_str());
        fout << "            <Include Dir=\"" + path + "\"/>\n";
        }
      fout << "        </Includes>\n";
      }
    fout << "    </Config>\n";
    }

  //ProcessConfigs(fout, target, mf);

  // Write Files section
  /* Use Default folders extensions
  {"Source Files", "*.c;*.C;*.cc;*.cpp;*.cp;*.cxx;*.c++;*.prg;*.pas;*.dpr;*.asm;*.s;*.bas;*.java;*.cs;*.sc;*.e;*.cob;*.html;*.rc;*.tcl;*.py;*.pl;*.d;*.m;*.mm;*.go"},
  {"Header Files", "*.h;*.H;*.hh;*.hpp;*.hxx;*.h++;*.inc;*.sh;*.cpy;*.if"},
  {"Resource Files","*.ico;*.cur;*.dlg"},
  {"Bitmaps","*.bmp"},
  {"Other Files",""},
  */
  std::set<std::string> source_ext;
  std::set<std::string> header_ext;
  std::set<std::string> resource_ext;
  std::set<std::string> bitmap_ext;

  source_ext.insert("c");
  source_ext.insert("cc");
  source_ext.insert("cpp");
  source_ext.insert("cp");
  source_ext.insert("cxx");
  source_ext.insert("c++");
  source_ext.insert("prg");
  source_ext.insert("pas");
  source_ext.insert("prg");
  source_ext.insert("dpr");
  source_ext.insert("asm");
  source_ext.insert("s");
  source_ext.insert("bas");
  source_ext.insert("java");
  source_ext.insert("cs");
  source_ext.insert("sc");
  source_ext.insert("e");
  source_ext.insert("cob");
  source_ext.insert("html");
  source_ext.insert("rc");
  source_ext.insert("rc");
  source_ext.insert("tcl");
  source_ext.insert("rc");
  source_ext.insert("py");
  source_ext.insert("rc");
  source_ext.insert("tcl");
  source_ext.insert("py");
  source_ext.insert("pl");
  source_ext.insert("d");
  source_ext.insert("m");
  source_ext.insert("mm");
  source_ext.insert("go");

  header_ext.insert("h");
  header_ext.insert("hh");
  header_ext.insert("hpp");
  header_ext.insert("hxx");
  header_ext.insert("h++");
  header_ext.insert("inc");
  header_ext.insert("sh");
  header_ext.insert("cpy");
  header_ext.insert("if");

  resource_ext.insert("ico");
  resource_ext.insert("cur");
  resource_ext.insert("dlg");

  bitmap_ext.insert("bmp");

  std::vector<std::string> source_filelist;
  std::vector<std::string> header_filelist;
  std::vector<std::string> resource_filelist;
  std::vector<std::string> bitmap_filelist;
  std::vector<std::string> other_filelist;

  std::string source_file;
  std::string ext;
  std::vector<cmSourceFile *> sources;
  target.GetSourceFiles(sources, buildType);
  for (std::vector<cmSourceFile *>::const_iterator sfIt = sources.begin(); sfIt != sources.end(); ++sfIt)
    {
    cmSourceFile *sf = *sfIt;
    if (sf->GetPropertyAsBool("GENERATED"))
      {
      continue;
      }
    source_file = sf->GetFullPath();
    ext = cmSystemTools::LowerCase(sf->GetExtension());

    if (source_ext.find(ext) != source_ext.end())
      {
      source_filelist.push_back(source_file);
      }
    else if (header_ext.find(ext) != header_ext.end())
      {
      header_filelist.push_back(source_file);
      }
    else if (resource_ext.find(ext) != resource_ext.end())
      {
      resource_filelist.push_back(source_file);
      }
    else if (bitmap_ext.find(ext) != bitmap_ext.end())
      {
      bitmap_filelist.push_back(source_file);
      }
    else
      {
      other_filelist.push_back(source_file);
      }
    }

  std::sort(source_filelist.begin(), source_filelist.end(), std::less<std::string>());
  std::sort(header_filelist.begin(), header_filelist.end(), std::less<std::string>());
  std::sort(resource_filelist.begin(), resource_filelist.end(), std::less<std::string>());
  std::sort(bitmap_filelist.begin(), bitmap_filelist.end(), std::less<std::string>());
  std::sort(other_filelist.begin(), other_filelist.end(), std::less<std::string>());

  std::vector<std::string> include_list;
  cmGeneratorTarget *gt = this->GlobalGenerator->GetGeneratorTarget(&target);
  mf->GetLocalGenerator()->GetIncludeDirectories(include_list, gt, "C", buildType);

  fout << "    <Files>\n";
  // Source Files
  fout << "        <Folder\n"
    "            Name=\"Source Files\"\n"
    "            Filters=\"*.c;*.C;*.cc;*.cpp;*.cp;*.cxx;*.c++;*.prg;*.pas;*.dpr;*.asm;*.s;*.bas;*.java;*.cs;*.sc;*.e;*.cob;*.html;*.rc;*.tcl;*.py;*.pl;*.d;*.m;*.mm;*.go\">\n";
  for (std::vector<std::string>::const_iterator it = source_filelist.begin(); it != source_filelist.end(); ++it)
    {
    source_file = cmSystemTools::RelativePath(project_path.c_str(), (*it).c_str());
    fout << "            <F N=\"" + source_file + "\"/>\n";
    }
  fout << "        </Folder>\n";

  // Header Files
  fout << "        <Folder\n"
    "            Name=\"Header Files\"\n"
    "            Filters=\"*.h;*.H;*.hh;*.hpp;*.hxx;*.h++;*.inc;*.sh;*.cpy;*.if\">\n";
  if (m_include_wildcards)
    {
    WriteIncludePathWildcards(fout, mf, project_path, include_list);
    }

  for (std::vector<std::string>::const_iterator it = header_filelist.begin(); it != header_filelist.end(); ++it)
    {
    source_file = cmSystemTools::RelativePath(project_path.c_str(), (*it).c_str());
    fout << "            <F N=\"" + source_file + "\"/>\n";
    }
  fout << "        </Folder>\n";

  // Resource Files
  fout << "        <Folder\n"
    "            Name=\"Resource Files\"\n"
    "            Filters=\"*.ico;*.cur;*.dlg\">\n";
  for (std::vector<std::string>::const_iterator it = resource_filelist.begin(); it != resource_filelist.end(); ++it)
    {
    source_file = cmSystemTools::RelativePath(project_path.c_str(), (*it).c_str());
    fout << "            <F N=\"" + source_file + "\"/>\n";
    }
  fout << "        </Folder>\n";

  // Bitmaps Files
  fout << "        <Folder\n"
    "            Name=\"Bitmaps\"\n"
    "            Filters=\"*.bmp\">\n";
  for (std::vector<std::string>::const_iterator it = bitmap_filelist.begin(); it != bitmap_filelist.end(); ++it)
    {
    source_file = cmSystemTools::RelativePath(project_path.c_str(), (*it).c_str());
    fout << "            <F N=\"" + source_file + "\"/>\n";
    }
  fout << "        </Folder>\n";

  // Other Files
  fout << "        <Folder\n"
    "            Name=\"Other Files\"\n"
    "            Filters=\"\">\n";
  for (std::vector<std::string>::const_iterator it = other_filelist.begin(); it != other_filelist.end(); ++it)
    {
    source_file = cmSystemTools::RelativePath(project_path.c_str(), (*it).c_str());
    fout << "            <F N=\"" + source_file + "\"/>\n";
    }
  fout << "        </Folder>\n";


  // write out first cmakefile
  // TBD:   include all list files?  is first one always root cmakefile?
  const char *cmakeRoot = mf->GetDefinition("CMAKE_ROOT");
  const std::vector<std::string>& list_files = mf->GetListFiles();
  if (list_files.size() > 0)
    {
    const std::string& cmakefile = list_files[0];
    source_file = cmSystemTools::RelativePath(project_path.c_str(), cmakefile.c_str());
    fout << "      <F N=\"" + source_file + "\"/>\n";
    }

  fout << "    </Files>\n";
  fout << "</Project>\n";

  m_project_files.insert(project_filename);
}

void cmExtraSlickEditGenerator::QueryCompiler(const cmMakefile *mf)
{
  // figure out which language to use
  // for now care only for C and C++
  std::string compilerIdVar = "CMAKE_CXX_COMPILER_ID";
  if (this->GlobalGenerator->GetLanguageEnabled("CXX") == false)
    {
    compilerIdVar = "CMAKE_C_COMPILER_ID";
    }

  std::string hostSystemName = mf->GetSafeDefinition("CMAKE_HOST_SYSTEM_NAME");
  std::string systemName = mf->GetSafeDefinition("CMAKE_SYSTEM_NAME");
  std::string compilerId = mf->GetSafeDefinition(compilerIdVar);
  std::string compiler = "gcc";  // default to gcc
  if (compilerId == "MSVC")
    {
    compiler = "msvc8";
    }
  else if (compilerId == "Borland")
    {
    compiler = "bcc";
    }
  else if (compilerId == "SDCC")
    {
    compiler = "sdcc";
    }
  else if (compilerId == "Intel")
    {
    compiler = "icc";
    }
  else if (compilerId == "Watcom" || compilerId == "OpenWatcom")
    {
    compiler = "ow";
    }
  else if (compilerId == "GNU")
    {
    compiler = "gcc";
    }

  std::cout << "Compiler Query: " << compiler << std::endl;
}

void cmExtraSlickEditGenerator::WriteProjectFileV2(const cmMakefile *mf,
                                                   std::set<std::string>& source_files,
                                                   std::set<std::string>& include_paths)
{
  std::string project_name = mf->GetProjectName();
  std::string project_path = mf->GetCurrentBinaryDirectory();
  std::string filename = project_path + "/" + project_name + ".vpj";
 
  cmGeneratedFileStream fout(filename.c_str());
  fout << "<!DOCTYPE Project SYSTEM \"http://www.slickedit.com/dtd/vse/10.0/vpj.dtd\">\n"
    "<Project\n"
    "    Version=\"10.0\"\n"
    "    VendorName=\"SlickEdit\"\n"
    "    TemplateName=\"CMake\"\n";
  fout << "    WorkingDir=\".\">\n";

  std::vector<std::string> configs;
  mf->GetConfigurations(configs);
  if (configs.empty())
    {
    configs.push_back("Release");
    }

  std::string buildType = mf->GetSafeDefinition("CMAKE_BUILD_TYPE");
  std::string command;

  for (std::vector<std::string>::const_iterator it = configs.begin(); it != configs.end(); ++it)
    {
    const std::string& config = (*it);
    fout << "    <Config\n";
    fout << "        Name=\"" + config + "\">\n";
    fout << "        <Menu>\n";
    
    command = GetBuildCommand(mf, "");
    WriteTargetCommand(fout, command, "Build", "&amp;Build", false);

    command = GetBuildCommand(mf, "clean");
    WriteTargetCommand(fout, command, "Clean", "&amp;Clean", false);

    fout << "        </Menu>\n";
    fout << "    </Config>\n";
    }
  
  std::set<std::string> source_ext;
  std::set<std::string> header_ext;
  std::set<std::string> resource_ext;
  std::set<std::string> bitmap_ext;

  source_ext.insert("c");
  source_ext.insert("cc");
  source_ext.insert("cpp");
  source_ext.insert("cp");
  source_ext.insert("cxx");
  source_ext.insert("c++");
  source_ext.insert("prg");
  source_ext.insert("pas");
  source_ext.insert("prg");
  source_ext.insert("dpr");
  source_ext.insert("asm");
  source_ext.insert("s");
  source_ext.insert("bas");
  source_ext.insert("java");
  source_ext.insert("cs");
  source_ext.insert("sc");
  source_ext.insert("e");
  source_ext.insert("cob");
  source_ext.insert("html");
  source_ext.insert("rc");
  source_ext.insert("rc");
  source_ext.insert("tcl");
  source_ext.insert("rc");
  source_ext.insert("py");
  source_ext.insert("rc");
  source_ext.insert("tcl");
  source_ext.insert("py");
  source_ext.insert("pl");
  source_ext.insert("d");
  source_ext.insert("m");
  source_ext.insert("mm");
  source_ext.insert("go");

  header_ext.insert("h");
  header_ext.insert("hh");
  header_ext.insert("hpp");
  header_ext.insert("hxx");
  header_ext.insert("h++");
  header_ext.insert("inc");
  header_ext.insert("sh");
  header_ext.insert("cpy");
  header_ext.insert("if");

  resource_ext.insert("ico");
  resource_ext.insert("cur");
  resource_ext.insert("dlg");

  bitmap_ext.insert("bmp");

  std::vector<std::string> source_filelist;
  std::vector<std::string> header_filelist;
  std::vector<std::string> resource_filelist;
  std::vector<std::string> bitmap_filelist;
  std::vector<std::string> other_filelist;

  std::string ext;
  for (std::set<std::string>::const_iterator
       it = source_files.begin();
       it != source_files.end(); 
       ++it)
    {
    filename = (*it);

    ext = cmSystemTools::GetFilenameLastExtension(filename);
    if (!ext.empty())
      {
      // remove '.', convert to lowcase
      ext = cmSystemTools::LowerCase(ext.substr(1));
      }

    if (source_ext.find(ext) != source_ext.end())
      {
      source_filelist.push_back(filename);
      }
    else if (header_ext.find(ext) != header_ext.end())
      {
      header_filelist.push_back(filename);
      }
    else if (resource_ext.find(ext) != resource_ext.end())
      {
      resource_filelist.push_back(filename);
      }
    else if (bitmap_ext.find(ext) != bitmap_ext.end())
      {
      bitmap_filelist.push_back(filename);
      }
    else
      {
      other_filelist.push_back(filename);
      }
    }

  std::sort(source_filelist.begin(), source_filelist.end(), std::less<std::string>());
  std::sort(header_filelist.begin(), header_filelist.end(), std::less<std::string>());
  std::sort(resource_filelist.begin(), resource_filelist.end(), std::less<std::string>());
  std::sort(bitmap_filelist.begin(), bitmap_filelist.end(), std::less<std::string>());
  std::sort(other_filelist.begin(), other_filelist.end(), std::less<std::string>());


  fout << "    <Files>\n";
  // Source Files
  fout << "        <Folder\n"
    "            Name=\"Source Files\"\n"
    "            Filters=\"*.c;*.C;*.cc;*.cpp;*.cp;*.cxx;*.c++;*.prg;*.pas;*.dpr;*.asm;*.s;*.bas;*.java;*.cs;*.sc;*.e;*.cob;*.html;*.rc;*.tcl;*.py;*.pl;*.d;*.m;*.mm;*.go\">\n";
  for (std::vector<std::string>::const_iterator it = source_filelist.begin(); it != source_filelist.end(); ++it)
    {
    filename = cmSystemTools::RelativePath(project_path.c_str(), (*it).c_str());
    fout << "            <F N=\"" + filename + "\"/>\n";
    }
  fout << "        </Folder>\n";

  // Header Files
  fout << "        <Folder\n"
    "            Name=\"Header Files\"\n"
    "            Filters=\"*.h;*.H;*.hh;*.hpp;*.hxx;*.h++;*.inc;*.sh;*.cpy;*.if\">\n";

  std::vector<std::string> include_list;
  for (std::set<std::string>::const_iterator
       it = include_paths.begin();
       it != include_paths.end(); 
       ++it)
    {
    include_list.push_back(*it);
    }
  WriteIncludePathWildcards(fout, mf, project_path, include_list);
  for (std::vector<std::string>::const_iterator it = header_filelist.begin(); it != header_filelist.end(); ++it)
    {
    filename = cmSystemTools::RelativePath(project_path.c_str(), (*it).c_str());
    fout << "            <F N=\"" + filename + "\"/>\n";
    }
  fout << "        </Folder>\n";

  // Resource Files
  fout << "        <Folder\n"
    "            Name=\"Resource Files\"\n"
    "            Filters=\"*.ico;*.cur;*.dlg\">\n";
  for (std::vector<std::string>::const_iterator it = resource_filelist.begin(); it != resource_filelist.end(); ++it)
    {
    filename = cmSystemTools::RelativePath(project_path.c_str(), (*it).c_str());
    fout << "            <F N=\"" + filename + "\"/>\n";
    }
  fout << "        </Folder>\n";

  // Bitmaps Files
  fout << "        <Folder\n"
    "            Name=\"Bitmaps\"\n"
    "            Filters=\"*.bmp\">\n";
  for (std::vector<std::string>::const_iterator it = bitmap_filelist.begin(); it != bitmap_filelist.end(); ++it)
    {
    filename = cmSystemTools::RelativePath(project_path.c_str(), (*it).c_str());
    fout << "            <F N=\"" + filename + "\"/>\n";
    }
  fout << "        </Folder>\n";

  // Other Files
  fout << "        <Folder\n"
    "            Name=\"Other Files\"\n"
    "            Filters=\"\">\n";
  for (std::vector<std::string>::const_iterator it = other_filelist.begin(); it != other_filelist.end(); ++it)
    {
    filename = cmSystemTools::RelativePath(project_path.c_str(), (*it).c_str());
    fout << "            <F N=\"" + filename + "\"/>\n";
    }
  fout << "        </Folder>\n";
  fout << "    </Files>\n";
  fout << "</Project>\n";
}

void cmExtraSlickEditGenerator::GenerateProjects()
{
  std::string workspace_path;
  std::string workspace_name;

  // loop projects and locate the root project.
  // and extract the information for creating the worspace
  for (std::map<std::string, std::vector<cmLocalGenerator*> >::const_iterator
       it = this->GlobalGenerator->GetProjectMap().begin();
       it!= this->GlobalGenerator->GetProjectMap().end();
       ++it)
    {
    const cmMakefile* mf =it->second[0]->GetMakefile();
    if (strcmp(mf->GetCurrentBinaryDirectory(),
               mf->GetHomeOutputDirectory()) == 0)
      {
      workspace_path = mf->GetCurrentBinaryDirectory();
      workspace_name = mf->GetProjectName();
      break;
      }
    }

  std::string filename = workspace_path + "/" + workspace_name + ".vpw";
  cmGeneratedFileStream wout(filename.c_str());
  if (!wout)
    {
    return;
    }

  // add workspace header
  wout << "<!DOCTYPE Workspace SYSTEM \"http://www.slickedit.com/dtd/vse/10.0/vpw.dtd\">\n"
    "<Workspace Version=\"10.0\" VendorName=\"SlickEdit\">\n"
    "    <Projects>\n";

  for (std::map<std::string, std::vector<cmLocalGenerator *> >::const_iterator
       it = this->GlobalGenerator->GetProjectMap().begin();
       it != this->GlobalGenerator->GetProjectMap().end();
       ++it)
    {
    const std::vector<cmLocalGenerator *>& gen = it->second;
    const cmMakefile* mf = gen[0]->GetMakefile();
    std::string project_name = mf->GetProjectName();
    std::string project_path = mf->GetCurrentBinaryDirectory();
   
    std::set<std::string> source_files;
    std::set<std::string> include_paths;

    for (std::vector<cmLocalGenerator *>::const_iterator i = gen.begin(); i != gen.end(); ++i)
      {
      const cmMakefile *lmf = (*i)->GetMakefile();
      const cmTargets& targets = lmf->GetTargets();
      for (cmTargets::const_iterator t = targets.begin(); t != targets.end(); ++t)
        {
        const cmMakefile *target_mf = t->second.GetMakefile();
        std::string path = target_mf->GetCurrentBinaryDirectory();
        std::string buildType = target_mf->GetSafeDefinition("CMAKE_BUILD_TYPE");

        bool build_sources = false;
        switch (t->second.GetType())
          {
          case cmTarget::EXECUTABLE:
          case cmTarget::STATIC_LIBRARY:
          case cmTarget::SHARED_LIBRARY:
          case cmTarget::MODULE_LIBRARY:
          case cmTarget::OBJECT_LIBRARY:
            build_sources = true;
            break;

          default:
            break;
          }

        if (build_sources)
          {
          std::string source_filename;
          std::vector<cmSourceFile *> sources;
          t->second.GetSourceFiles(sources, buildType);
          for (std::vector<cmSourceFile *>::const_iterator sfIt = sources.begin(); sfIt != sources.end(); sfIt++)
            {
            cmSourceFile *sf = *sfIt;
            if (sf->GetPropertyAsBool("GENERATED"))
              {
              continue;
              }
            source_filename = sf->GetFullPath();
            if (source_files.find(source_filename) == source_files.end())
              {
              source_files.insert(source_filename);
              }
            }

          std::vector<std::string> include_list;
          cmGeneratorTarget *gt = this->GlobalGenerator->GetGeneratorTarget(&t->second);
          target_mf->GetLocalGenerator()->GetIncludeDirectories(include_list, gt, "C", buildType);
          for (std::vector<std::string>::const_iterator inc_t = include_list.begin(); inc_t != include_list.end(); ++inc_t)
            {
            if (include_paths.find(*inc_t) == include_paths.end())
              {
              include_paths.insert(*inc_t);
              }
            }
          }
        }
	  }
      WriteProjectFileV2(mf, source_files, include_paths);
      filename = project_path + "/" + project_name + ".vpj";
      filename = cmSystemTools::RelativePath(workspace_path.c_str(), filename.c_str());
      wout << "		<Project File=\"" + filename + "\"/>\n";
    }

  wout << "    </Projects>\n";
  wout << "</Workspace>\n";
}
